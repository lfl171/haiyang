# BLACK TIDE · 黑潮

> 一个用 UE5.8 C++ 从零搭建的**暴雨海洋实时电影场景**。
> 风暴、海面、船体、闪电、镜头全部由数据驱动、逐帧联动，而不是摆在关卡里的静态布景。

![Engine](https://img.shields.io/badge/Unreal-5.8-0E1128?logo=unrealengine)
![Language](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)
![Platform](https://img.shields.io/badge/Platform-Win64-0078D6)

---

## 这是什么

暴雨夜晚的海面上，一艘海盗船在浪涌中穿行，闪电撕裂天空。

项目刻意避开了「在关卡里手摆一堆 Actor、调好参数就交差」的做法。天气是一个可插值的状态机，海面高度同时供渲染和物理采样，船的姿态由四探针真实浮力计算得出，闪电强度按概率roll出四档再走多段闪光曲线。所有基础资产由 Python 脚本幂等生成，改天气只需要换一份数据资产。

整个工程约 **2000 行 C++**（19 个源文件）+ 3 个 Python 自动化脚本（约 700 行）。

## 技术栈

| 项 | 说明 |
|----|------|
| 引擎 | Unreal Engine 5.8 |
| 语言 | C++20（模块 `BlackTide`）+ Python（编辑器自动化） |
| 渲染 | Lumen 动态全局光照 / 虚拟阴影贴图（VSM）/ 硬件光追 |
| 水体 | 自研三层波形系统（`ProceduralMesh` + WPO 顶点位移） |
| 特效 | Niagara、材质参数集（MPC）全局参数总线 |
| 插件 | Water、Niagara、MovieRenderPipeline、PythonScriptPlugin |

## 架构总览

```
┌─ Presentation ── Sequencer 电影镜头 / Debug HUD
├─ Scene ───────── Ship / Ocean / Sky / Lighting
├─ Effects ─────── Niagara 雨雪浪花 / 闪电事件链
├─ Rendering ───── 材质体系 / MPC 全局参数 / 后处理
└─ Core System ─── StormManager / OceanManager / CinematicDirector
```

核心系统全部是 **Manager 模式**的单例 Actor，关卡中各放一个实例，彼此通过委托广播解耦 —— 没有任何系统直接引用另一个系统。

| 系统 | 类 | 职责 |
|------|-----|------|
| 风暴总控 | `ABTStormManager` | 天气状态机、参数插值、MPC 写入、委托广播 |
| 天气档案 | `UBTStormProfile` | `UDataAsset`，一份资产描述一种天气 |
| 海洋总控 | `ABTOceanManager` | 三层波形求值，向船体提供水面高度采样 |
| 海面网格 | `ABTOceanSurfaceActor` | 程序化生成高细分平面，承载 WPO 位移 |
| 海盗船 | `ABTShipActor` | 船体 + 4 个功能组件 |
| 闪电 | `ABTLightningController` | 概率调度 + 多段闪光曲线 |
| 电影调度 | `ABTCinematicDirector` | 定时触发闪电/天气/Sequencer |
| 调试 | `ABTDebugHUD` / `BTGameMode` / `BTPlayerController` | 实时参数面板（F1 开关） |

## 核心系统

### 数据驱动的风暴系统

天气不是写死的参数，而是 6 份 `DA_StormProfile` 数据资产：

```
Clear → Cloudy → LightRain → HeavyRain → Storm → ExtremeStorm
```

每份档案描述 **9 维参数**（雨量 / 风力 / 浪高 / 浪速 / 雾 / 闪电频率 / 海面暗度 / 飞溅 / 泡沫）、闪电间隔与过渡时长。切换天气时 `BTStormManager` 对参数做线性插值，并在过渡中途被打断时重新捕获起点，因此**连续切换不会回弹**。

每帧做两件事：

1. 广播 `OnStormParamsUpdated` 委托 → Ocean / Lightning / Sail / Wetness / ShipFX 各自订阅
2. 把参数写入 **MPC 全局材质参数集** → 所有材质同帧响应

> 改天气 = 换一份数据资产，零代码。

### 三层波形海洋

海面由三层不同频率的波叠加：

| 层 | 振幅 | 波长 | 速度 | 作用 |
|----|------|------|------|------|
| Large | 3.0 m | 120 m | 1.0× | 大尺度涌浪，远处主导 |
| Medium | 1.2 m | 32 m | 1.3× | 明显波峰，中景主体 |
| Detail | 0.25 m | 7 m | 1.8× | 细碎水面，近处细节 |

单层求值（深水色散关系给出物理正确的相速度，`Q` 为波峰尖锐度）：

```
K = 2π / L                                    // 波数
C = √(9.8 / K) × Speed                        // 深水波相速度
φ = K · (D·P − C·t)                           // 相位
h = lerp(sin φ, |sin φ|^(1+2Q) · sign(sin φ), Q) × A
```

关键在于**同一份公式跑两遍**：C++ 侧 `BTOceanManager::GetWaterHeightAt()` 供船体浮力采样，GPU 侧 `M_Ocean` 用 Custom HLSL 节点做顶点位移。两侧共用同一套波参数与同一个 MPC，改一处两边同步。

船体姿态因此是**真实计算**出来的，而不是叠加 Sin 函数假摇晃：四探针分别取船艏/船艉/左舷/右舷的水面高度，由高度差算出 Pitch / Roll，再由 `FInterpTo` 做阻尼平滑。

### 船体组件

`ABTShipActor` 挂载 4 个功能组件，各自订阅风暴委托：

| 组件 | 行为 |
|------|------|
| `UShipMovementComponent` | 4 探针浮力 → Heave / Pitch / Roll |
| `USailComponent` | 风力驱动帆面鼓动（`WindSway` / `Billow`）+ 阵风横倾 |
| `UWetnessComponent` | 淋湿积分：下雨累积、停雨缓慢晾干，驱动材质湿润度 |
| `UShipFXComponent` | 按航速与浪高触发船艏浪花与船迹泡沫 |

湿润模型有个细节值得一提：**淋湿快、晾干慢**。满雨强下约 20 秒湿透，但完全晾干要 200 秒 —— 这个不对称让雨停后的船体依然「湿漉漉」，比对称插值自然得多。

### 闪电

闪电不是简单闪一下，而是一条 0.30 秒的**多段曲线**：

```
0.00–0.03s  线性冲顶   0 → 1.0     ← 爆闪
0.03–0.08s  快速衰减   1.0 → 0.2   ← 暗下
0.08–0.15s  二次爬升   0.2 → 1.0   ← 回闪
0.15–0.30s  完全衰减   1.0 → 0.0
```

强度分 Weak / Normal / Strong / Extreme 四档（峰值 3.5 万 ~ 20 万 lux），按当前闪电频率做概率 roll —— 天气越恶劣，高烈度闪电越常见。间隔同样随频率收缩：Clear 时约 45~90 秒一次，ExtremeStorm 时 2~6 秒一次。

## 资产自动化管线

关卡里的基础资产**全部由 Python 命令行一键生成**，幂等、可重跑：

```bash
UnrealEditor-Cmd.exe BlackTide.uproject -NullRHI -unattended -nosplash \
    -run=pythonscript -script=D:\BTScripts\setup_blacktide.py
```

一次执行产出：

- `MPC_Global` —— 8 个标量参数的材质参数集
- 6 份 `DA_Storm_*` 数据资产（含调好的参数表）
- `M_Master` 湿润材质体系 + `MI_Wood` / `MI_Metal` / `MI_Cloth`
- `M_Ocean` —— 三层波形 WPO 位移 + 泡沫遮罩
- 整个主关卡：管理器摆位、月光基调、雾、后处理、PlayerStart

脚本通过 `does_asset_exist → load_asset / create_asset` 保证幂等，并且每次都重新写入数值（而非只在创建时写），所以**重复执行是收敛的**：无论资产是新建的还是已存在的，跑完都是同一个状态。

> ⚠️ 脚本必须放在**无空格路径**下执行。UE 命令行的 `-script=` 参数遇到带空格的路径会吞掉转义符（`\U` → `å`），脚本会静默不执行。

## 编译与运行

**环境要求**：UE 5.8、Visual Studio 2022（含 C++ 游戏开发工作负载，见 `.vsconfig`）、Windows 10/11 SDK。

```bash
# 1. 生成工程文件
"D:\game\UE_5.8\Engine\Build\BatchFiles\Build.bat" BlackTideEditor Win64 Development \
    -Project="<PROJECT_DIR>\BlackTide.uproject" -WaitMutex

# 2. 打开编辑器
"D:\game\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "<PROJECT_DIR>\BlackTide.uproject"
```

打开后加载关卡 `Content/BLACKTIDE/Maps/L_BlackTide_Main`，`F1` 开关实时调试面板。

> 首次克隆后 `Binaries/` 与 `Intermediate/` 不存在，必须先跑一次上面第 1 步的编译，编辑器才能加载 `BlackTide` 模块。

## 目录结构

```
BlackTide.uproject
Config/                        引擎与输入配置
Content/BLACKTIDE/
  ├─ Core/Data/                DA_Storm_* 天气档案
  ├─ Environment/Ocean/        M_Ocean 海面材质
  ├─ Materials/                MPC_Global / M_Master / MI_*
  ├─ Ship/                     BP_PirateShip + 船体网格
  └─ Maps/                     L_BlackTide_Main
Source/BlackTide/
  ├─ Core/                     BTStormManager / BTStormProfile / BTWeatherTypes
  ├─ Ocean/                    BTOceanManager / BTOceanSurfaceActor
  ├─ Ship/                     BTShipActor + 4 组件
  ├─ FX/                       BTLightningController
  ├─ Cinematic/                BTCinematicDirector
  └─ Debug/                    BTDebugHUD / BTGameMode / BTPlayerController
Scripts/                       Python 资产自动化管线
Docs/                          开发日志
```

## 工程笔记

### 海面网格为什么不在编辑器里生成

海面是 512×512 段的程序化网格（约 26 万顶点、157 万索引），原先在 `OnConstruction` 里生成。问题在于 **ProceduralMesh 的顶点数据会被序列化进 `.umap`** —— 实测把关卡从几 MB 撑到 **162 MB**，超过 GitHub 单文件 100 MB 的硬上限，`push` 直接被拒。

现在的做法是分两档生成：

- **编辑器**（`OnConstruction`）：只生成 96 段低精度预览网格，够看清 WPO 波形即可
- **运行时**（`BeginPlay`）：重建 512 段全精度网格，运行时数据不落盘

关卡体积因此回到 **5.8 MB**（-96%），仓库可以直接 `git clone`，不需要 Git LFS。两条路径都用 `IsGameWorld()` 隔开，运行时不会重复生成。

### 米与厘米

UE 世界单位是厘米，但波参数是按米设计的。不换算的话 3 m 大浪渲染出来只有 3 cm —— 一片死水。转换统一放在采样入口：`GetWaterHeightAt()` 进 `cm → m`，出 `m → cm`，中间全按米算。

### 引擎自带 Plane 只有 4 个顶点

WPO 顶点位移需要足够密的网格，4 个顶点的平面无处施展。所以自研 `BTOceanSurfaceActor` 用 `ProceduralMesh` 生成网格。1200 m 覆盖范围 / 512 段 = 单元格 2.34 m，对最短的 7 m Detail 波刚好约 3 顶点每波长，满足奈奎斯特采样。

## 现状与路线图

**已实现**

- 6 档天气状态机 + 参数插值 + MPC 全局参数总线
- 三层波形海洋：CPU 采样与 GPU 顶点位移共用同一套公式
- 四探针浮力船体 + 帆控 / 湿润 / 浪花三个组件
- 0.30 s 多段闪电曲线 + 四档强度概率调度
- F1 实时调试面板（FPS / 风暴参数 / 船体姿态）
- Python 幂等资产管线

**进行中 / 待补**

- [ ] Niagara 分层雨（远 / 中 / 近）+ 船艏浪花 + 船迹泡沫
- [ ] Sequencer 45 秒六分镜（0–7 s 闪电揭示 → 38–45 s 英雄镜头 + 标题）
- [ ] 雷声与风雨音频（按距离延迟雷声）
- [ ] 把 `MI_Wood` / `MI_Metal` / `MI_Cloth` 接到船体网格上，让湿润系统真正生效
- [ ] Fab 高精度船模替换（管线不变，即插即用）
- [ ] 天气切换的玩家输入绑定

## 致谢

- 海盗船模型来自 [Kenney Pirate Kit](https://kenney.nl/assets/pirate-kit)（CC0，公共领域）
- 引擎：Unreal Engine 5.8 © Epic Games

---

开发日志见 [`Docs/BLACKTIDE_DevBlog_01.md`](Docs/BLACKTIDE_DevBlog_01.md)。
