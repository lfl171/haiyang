# BLACK TIDE · 黑潮 —— UE5.8 暴雨海盗船实时电影场景开发日志（一）

> 从零搭建一个企业级「暴雨海洋生态系统」：不是摆一个静态场景，而是让风暴、海洋、船体、闪电、镜头全部由数据驱动、实时联动。

![BLACK TIDE 场景实拍](images/blacktide_01.png)

## 项目定位

- **引擎**：Unreal Engine 5.8（Lumen 硬件光追 + Nanite + VSM + DX12）
- **形态**：45 秒实时渲染电影短片 + 可交互调试场景
- **核心命题**：暴雨夜晚的海面上，一艘海盗船在浪涌中穿行，闪电撕裂天空

## 五层架构

```
Presentation 层  —— Sequencer 电影镜头 / Debug HUD
Scene 层         —— Ship / Ocean / Sky / Lighting
Effects 层       —— Niagara 雨雪浪花 / 闪电事件链
Rendering 层     —— 材质体系 / MPC 全局参数 / 后处理
Core System 层   —— StormManager / OceanManager / CinematicDirector
```

所有核心系统采用 **Manager 模式**单例运转，关卡中各放一个实例，通过委托广播解耦。

## 数据驱动的风暴系统

天气不是写死的参数，而是 6 份 `DA_StormProfile` 数据资产：

```
Clear → Cloudy → LightRain → HeavyRain → Storm → ExtremeStorm
```

每份 Profile 描述 9 维归一化参数（雨量/风力/浪高/浪速/雾/闪电频率/海面暗度/飞溅/泡沫）+ 闪电间隔 + 过渡时长。`BT_StormManager` 在状态切换时对参数做平滑插值，每帧做两件事：

1. 广播 `OnStormParamsUpdated` 委托 → Ocean/Lightning/FX 各自订阅
2. 把参数写入 **MPC 全局材质参数集** → 所有材质同帧响应

改天气 = 换一份数据资产，零代码。

## 三层 Gerstner 海洋：CPU 与 GPU 的严格一致

海面由三层不同频率的波叠加：

| 层 | 振幅 | 波长 | 作用 |
|----|------|------|------|
| Large | 3.0m | 120m | 大尺度涌浪，远处主导 |
| Medium | 1.2m | 32m | 明显波峰，中景主体 |
| Detail | 0.25m | 7m | 细碎水面，近处细节 |

每层公式（色散关系保证物理正确的相速度）：

```
K = 2π/L                    // 波数
C = √(9.8/K) × Speed        // 深水波相速度
φ = K·(D·P − C·t)           // 相位
h = lerp(sin φ, |sin φ|^(1+2Q)·sign, Q) × A   // Q=尖锐度
```

关键在于**同一份公式跑两遍**：C++ 侧 `BTOceanManager::GetWaterHeightAt()` 供船体四探针浮力采样（真实的 Heave/Pitch/Roll，而不是 Sin 函数假摇晃）；GPU 侧 `M_Ocean` 用 Custom HLSL 节点做顶点位移。两边参数、公式逐行一致，视觉与物理永不脱节。

## 踩坑实录

1. **米与厘米的陷阱**：UE 世界单位是厘米，波参数按米设计。不换算的话 3m 大浪渲染出来只有 3cm——一片死水。修复点在采样入口统一 `cm→m→cm`。
2. **4 顶点的海面**：引擎自带 Plane 只有 4 个顶点，WPO 顶点位移无处施展。自研 `BTOceanSurfaceActor` 用 ProceduralMesh 生成 512×512 网格（单元格 2.3m，满足 Detail 波 7m 波长的奈奎斯特采样）。
3. **命令行路径转义**：UE 命令行的 `-script=` 参数遇到带空格路径会被吞掉转义符（`\U` → `å`）。Python 自动化脚本必须放无空格路径执行。
4. **Interchange 拆网格**：UE5.8 导入多网格 FBX 会自动拆成独立资产并忽略目标命名——反而意外之喜，船帆因此成为独立网格，可以直接挂进 `SailComponent` 做风力鼓动。

## 自动化管线

所有基础资产都不是手点的，而是 Python 命令行一键生成（幂等可重跑）：

```
UnrealEditor-Cmd.exe BlackTide.uproject -run=pythonscript -script=setup_blacktide.py
```

一次执行产出：MPC 参数集、6 份风暴 Profile、M_Master 湿润材质体系（Wetness 变暗 + WaterFilm 高光）、三层 Gerstner 海洋材质、整个主关卡（管理器摆位 + 灯光基调 + GameMode）。

## 当前效果

- 风暴状态初始化为 Storm：4.5m 涌浪、浪尖白沫由 FoamIntensity 驱动
- 灯光基调：深蓝/黑为主，闪电冷白（LightningController 多段闪光曲线：0.03s 爆闪 → 0.08s 暗下 → 0.15s 二次闪 → 0.30s 恢复）
- 海盗船：Kenney Pirate Kit（CC0）大型海盗船，按包围盒自动缩放至 40m 船长，已装配浮力/帆控/湿润/浪花四个组件

## 下一步

- [ ] Niagara 分层雨（NS_Rain_Far/Mid/Near）+ 船艏浪花 + 船迹泡沫
- [ ] Sequencer 45 秒六分镜（0-7s 闪电揭示 → 38-45s 英雄镜头 + 标题）
- [ ] 雷声/风雨音频（闪电按距离延迟雷声）
- [ ] Fab 高精度船模替换（管线不变，即插即用）

---

*工程已开源，欢迎交流。*
