# -*- coding: utf-8 -*-
"""
BLACK TIDE - 编辑器资产一键生成脚本（幂等，可重复运行）

通过命令行执行：
  UnrealEditor-Cmd.exe BlackTide.uproject -NullRHI -unattended -run=pythonscript -script=setup_blacktide.py

生成内容：
  1. MPC_Global 材质参数集（8 个标量，名称与 BTStormManager::ApplyParamsToMPC 一致）
  2. 6 个 DA_StormProfile 数据资产（Clear..ExtremeStorm）
  3. M_Master 基础材质（Wetness/WaterFilm 湿润体系）+ MI_Wood/MI_Metal/MI_Cloth
  4. M_Ocean 海洋材质（三层 Gerstner WPO，与 BTOceanManager CPU 采样严格一致）
  5. L_BlackTide_Main 关卡（管理器摆位 + 海面 + 灯光基调）
"""

import json
import traceback
import unreal

REPORT = {"steps": [], "created": [], "errors": []}
REPORT_PATH = r"E:\UE5 CPP\T_2\Scripts\last_run_report.json"

MEL = unreal.MaterialEditingLibrary

PATH_MATERIALS = "/Game/BLACKTIDE/Materials"
PATH_PROFILES = "/Game/BLACKTIDE/Core/Data"
PATH_OCEAN = "/Game/BLACKTIDE/Environment/Ocean"
PATH_MAP = "/Game/BLACKTIDE/Maps/L_BlackTide_Main"


# ---------------------------------------------------------------- 工具

def step(name, ok, detail=""):
    REPORT["steps"].append({"name": name, "ok": bool(ok), "detail": str(detail)})
    unreal.log("[BT] %s => %s %s" % (name, "OK" if ok else "FAIL", detail))


def save(obj):
    unreal.EditorAssetLibrary.save_asset(obj.get_path_name().split(".")[0])


def ensure_asset(name, package_path, asset_class, factory=None):
    """存在则加载，不存在则创建。"""
    full = "%s/%s" % (package_path, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return unreal.EditorAssetLibrary.load_asset(full)
    obj = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, package_path, asset_class, factory)
    if obj is None:
        raise RuntimeError("create_asset failed: " + full)
    REPORT["created"].append(full)
    return obj


def expr(mat, cls, x, y, **props):
    e = MEL.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def connect(a, b, a_out="", b_in=""):
    MEL.connect_material_expressions(a, a_out, b, b_in)


def to_prop(e, prop):
    MEL.connect_material_property(e, "", prop)


def collection_param(mat, mpc, name, x, y):
    e = expr(mat, unreal.MaterialExpressionCollectionParameter, x, y, parameter_name=name)
    e.set_editor_property("collection", mpc)
    return e


# ---------------------------------------------------------------- 1. MPC

MPC_SCALARS = [
    ("RainIntensity", 0.9), ("WindStrength", 0.85), ("WaveHeight", 1.5), ("WaveSpeed", 1.3),
    ("FogDensity", 0.7), ("OceanDarkness", 0.85), ("FoamIntensity", 0.85), ("SplashIntensity", 0.85),
]


def setup_mpc():
    mpc = ensure_asset("MPC_Global", PATH_MATERIALS, unreal.MaterialParameterCollection)
    params = []
    for n, v in MPC_SCALARS:
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", n)
        p.set_editor_property("default_value", v)
        params.append(p)
    mpc.set_editor_property("scalar_parameters", params)
    save(mpc)
    return mpc


# ---------------------------------------------------------------- 2. Storm Profiles

# 注意：WaveHeight 是振幅倍率（BTOceanManager::OnStormParamsReceived 直接相乘），
# Storm 1.5 => 大涌浪 3m*1.5 = 4.5m；ExtremeStorm 2.2 => 6.6m
PROFILES = {
    "Clear":        dict(state="CLEAR",         rain=0.0,  wind=0.10, wave_h=0.30, wave_s=0.80, fog=0.05, light=0.0,  dark=0.05, splash=0.05, foam=0.10, lmin=45.0, lmax=90.0, trans=10.0),
    "Cloudy":       dict(state="CLOUDY",        rain=0.0,  wind=0.25, wave_h=0.50, wave_s=0.90, fog=0.15, light=0.05, dark=0.25, splash=0.10, foam=0.20, lmin=30.0, lmax=60.0, trans=10.0),
    "LightRain":    dict(state="LIGHT_RAIN",    rain=0.30, wind=0.40, wave_h=0.70, wave_s=1.00, fog=0.30, light=0.15, dark=0.45, splash=0.30, foam=0.35, lmin=15.0, lmax=35.0, trans=8.0),
    "HeavyRain":    dict(state="HEAVY_RAIN",    rain=0.70, wind=0.65, wave_h=1.00, wave_s=1.15, fog=0.50, light=0.45, dark=0.65, splash=0.60, foam=0.60, lmin=8.0,  lmax=20.0, trans=8.0),
    "Storm":        dict(state="STORM",         rain=0.90, wind=0.85, wave_h=1.50, wave_s=1.30, fog=0.70, light=0.75, dark=0.85, splash=0.85, foam=0.85, lmin=4.0,  lmax=12.0, trans=6.0),
    "ExtremeStorm": dict(state="EXTREME_STORM", rain=1.0,  wind=1.00, wave_h=2.20, wave_s=1.50, fog=0.85, light=1.0,  dark=1.00, splash=1.00, foam=1.00, lmin=2.0,  lmax=6.0,  trans=5.0),
}


def make_params(d):
    p = unreal.BTStormParams()
    p.set_editor_property("rain_intensity", d["rain"])
    p.set_editor_property("wind_strength", d["wind"])
    p.set_editor_property("wave_height", d["wave_h"])
    p.set_editor_property("wave_speed", d["wave_s"])
    p.set_editor_property("fog_density", d["fog"])
    p.set_editor_property("lightning_frequency", d["light"])
    p.set_editor_property("ocean_darkness", d["dark"])
    p.set_editor_property("splash_intensity", d["splash"])
    p.set_editor_property("foam_intensity", d["foam"])
    return p


def setup_profiles():
    cls = unreal.load_class(None, "/Script/BlackTide.BTStormProfile")
    result = {}
    for name, d in PROFILES.items():
        asset_name = "DA_Storm_" + name
        full = PATH_PROFILES + "/" + asset_name
        obj = None
        if unreal.EditorAssetLibrary.does_asset_exist(full):
            obj = unreal.EditorAssetLibrary.load_asset(full)
        else:
            try:
                f = unreal.DataAssetFactory()
                try:
                    f.set_editor_property("data_asset_class", cls)
                except Exception:
                    pass
                obj = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                    asset_name, PATH_PROFILES, unreal.DataAsset, f)
            except Exception:
                obj = None
            if obj is None:
                obj = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                    asset_name, PATH_PROFILES, cls, None)
            REPORT["created"].append(full)

        obj.set_editor_property("weather_state", getattr(unreal.BTWeatherState, d["state"]))
        obj.set_editor_property("params", make_params(d))
        obj.set_editor_property("lightning_min_interval", d["lmin"])
        obj.set_editor_property("lightning_max_interval", d["lmax"])
        obj.set_editor_property("transition_duration", d["trans"])
        save(obj)
        result[name] = obj
    return result


# ---------------------------------------------------------------- 3. M_Master + 实例

def setup_master():
    mat = ensure_asset("M_Master", PATH_MATERIALS, unreal.Material)

    base_color = expr(mat, unreal.MaterialExpressionVectorParameter, -700, -100,
                      parameter_name="BaseColor", default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    rough_p = expr(mat, unreal.MaterialExpressionScalarParameter, -700, 250, parameter_name="Roughness", default_value=0.7)
    metal_p = expr(mat, unreal.MaterialExpressionScalarParameter, -700, 350, parameter_name="Metallic", default_value=0.0)
    wet_p = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 120, parameter_name="Wetness", default_value=0.0)
    film_p = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 480, parameter_name="WaterFilm", default_value=0.0)

    # Albedo *= lerp(1, 0.55, Wetness)
    one = expr(mat, unreal.MaterialExpressionConstant, -900, 30, r=1.0)
    dark_k = expr(mat, unreal.MaterialExpressionConstant, -900, 70, r=0.55)
    lerp_dark = expr(mat, unreal.MaterialExpressionLinearInterpolate, -500, 60)
    connect(one, lerp_dark, "", "A")
    connect(dark_k, lerp_dark, "", "B")
    connect(wet_p, lerp_dark, "", "Alpha")
    mul_alb = expr(mat, unreal.MaterialExpressionMultiply, -250, -60)
    connect(base_color, mul_alb, "", "A")
    connect(lerp_dark, mul_alb, "", "B")
    to_prop(mul_alb, unreal.MaterialProperty.MP_BASE_COLOR)

    # Roughness = lerp(Roughness, 0.12, saturate(Wetness + WaterFilm))
    add_w = expr(mat, unreal.MaterialExpressionAdd, -700, 450)
    connect(wet_p, add_w, "", "A")
    connect(film_p, add_w, "", "B")
    sat_w = expr(mat, unreal.MaterialExpressionSaturate, -500, 450)
    connect(add_w, sat_w)
    wet_rough = expr(mat, unreal.MaterialExpressionConstant, -500, 520, r=0.12)
    lerp_rough = expr(mat, unreal.MaterialExpressionLinearInterpolate, -250, 300)
    connect(rough_p, lerp_rough, "", "A")
    connect(wet_rough, lerp_rough, "", "B")
    connect(sat_w, lerp_rough, "", "Alpha")
    to_prop(lerp_rough, unreal.MaterialProperty.MP_ROUGHNESS)

    to_prop(metal_p, unreal.MaterialProperty.MP_METALLIC)

    # Specular = lerp(0.5, 1.0, WaterFilm)
    half = expr(mat, unreal.MaterialExpressionConstant, -500, 620, r=0.5)
    one2 = expr(mat, unreal.MaterialExpressionConstant, -500, 660, r=1.0)
    lerp_spec = expr(mat, unreal.MaterialExpressionLinearInterpolate, -250, 550)
    connect(half, lerp_spec, "", "A")
    connect(one2, lerp_spec, "", "B")
    connect(film_p, lerp_spec, "", "Alpha")
    to_prop(lerp_spec, unreal.MaterialProperty.MP_SPECULAR)

    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    save(mat)
    return mat


def setup_instances(master):
    defs = {
        "MI_Wood":  dict(base=unreal.LinearColor(0.10, 0.055, 0.025, 1.0), rough=0.80, metal=0.0),
        "MI_Metal": dict(base=unreal.LinearColor(0.16, 0.17, 0.18, 1.0),   rough=0.35, metal=1.0),
        "MI_Cloth": dict(base=unreal.LinearColor(0.12, 0.018, 0.014, 1.0), rough=0.85, metal=0.0),
    }
    for name, d in defs.items():
        full = PATH_MATERIALS + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(full):
            inst = unreal.EditorAssetLibrary.load_asset(full)
        else:
            f = unreal.MaterialInstanceConstantFactoryNew()
            try:
                f.set_editor_property("initial_parent", master)
            except Exception:
                pass
            inst = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, PATH_MATERIALS, unreal.MaterialInstanceConstant, f)
            REPORT["created"].append(full)
        try:
            MEL.set_material_instance_vector_parameter_value(inst, "BaseColor", d["base"])
            MEL.set_material_instance_scalar_parameter_value(inst, "Roughness", d["rough"])
            MEL.set_material_instance_scalar_parameter_value(inst, "Metallic", d["metal"])
        except Exception as ex:
            REPORT["errors"].append("instance params %s: %s" % (name, ex))
        save(inst)


# ---------------------------------------------------------------- 4. M_Ocean

# 与 BTOceanManager::SampleWaveLayer 严格一致：
#   K=2pi/L, C=sqrt(9.8/K)*Speed, ph=K*(dot(D,P)-C*t)
#   s=sin(ph), sh=|s|^(1+2Q)*sign(s), h += lerp(s,sh,Q)*A
# 单位：P 换算为米，输出高度乘 100 回到厘米；WaveTime 由 Time*WaveSpeed 预乘
GERSTNER_COMMON = r"""
float2 P = WorldPos.xy * 0.01;
float t = WaveTime;
float h = 0.0;
{
    float2 D = normalize(float2(1.0, 0.15));
    float K = 6.28318530718 / 120.0;
    float C = sqrt(9.8 / K) * 1.0;
    float ph = K * (dot(D, P) - C * t);
    float s = sin(ph);
    float sh = pow(abs(s), 1.8) * sign(s);
    h += lerp(s, sh, 0.4) * 3.0;
}
{
    float2 D = normalize(float2(0.9, -0.4));
    float K = 6.28318530718 / 32.0;
    float C = sqrt(9.8 / K) * 1.3;
    float ph = K * (dot(D, P) - C * t);
    float s = sin(ph);
    float sh = pow(abs(s), 2.1) * sign(s);
    h += lerp(s, sh, 0.55) * 1.2;
}
{
    float2 D = normalize(float2(0.6, 0.8));
    float K = 6.28318530718 / 7.0;
    float C = sqrt(9.8 / K) * 1.8;
    float ph = K * (dot(D, P) - C * t);
    float s = sin(ph);
    float sh = pow(abs(s), 1.6) * sign(s);
    h += lerp(s, sh, 0.3) * 0.25;
}
"""


def make_custom(mat, x, y, code, out_type):
    e = expr(mat, unreal.MaterialExpressionCustom, x, y)
    inputs = []
    for n in ("WorldPos", "WaveTime", "WaveHeight"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        inputs.append(ci)
    e.set_editor_property("inputs", inputs)
    e.set_editor_property("output_type", out_type)
    e.set_editor_property("code", code)
    e.set_editor_property("description", "Gerstner3Layer")
    return e


def setup_ocean(mpc):
    mat = ensure_asset("M_Ocean", PATH_OCEAN, unreal.Material)

    wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1100, -150)
    tm = expr(mat, unreal.MaterialExpressionTime, -1100, 50)
    c_speed = collection_param(mat, mpc, "WaveSpeed", -1100, 200)
    c_wh = collection_param(mat, mpc, "WaveHeight", -1100, 350)
    c_foam = collection_param(mat, mpc, "FoamIntensity", -1100, 650)
    c_dark = collection_param(mat, mpc, "OceanDarkness", -500, 850)

    mul_t = expr(mat, unreal.MaterialExpressionMultiply, -800, 100)
    connect(tm, mul_t, "", "A")
    connect(c_speed, mul_t, "", "B")

    # WPO（float3，单位厘米）
    wpo = make_custom(mat, -350, -150, GERSTNER_COMMON + "\nreturn float3(0.0, 0.0, h * WaveHeight * 100.0);\n",
                      unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    connect(wp, wpo, "", "WorldPos")
    connect(mul_t, wpo, "", "WaveTime")
    connect(c_wh, wpo, "", "WaveHeight")
    to_prop(wpo, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)

    # 高度（米，float），供泡沫遮罩
    hgt = make_custom(mat, -350, 450, GERSTNER_COMMON + "\nreturn h * WaveHeight;\n",
                      unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    connect(wp, hgt, "", "WorldPos")
    connect(mul_t, hgt, "", "WaveTime")
    connect(c_wh, hgt, "", "WaveHeight")

    # foamMask = saturate((h - 0.4) / 1.5) * FoamIntensity
    k04 = expr(mat, unreal.MaterialExpressionConstant, -350, 600, r=0.4)
    sub = expr(mat, unreal.MaterialExpressionSubtract, -150, 500)
    connect(hgt, sub, "", "A")
    connect(k04, sub, "", "B")
    k15 = expr(mat, unreal.MaterialExpressionConstant, -350, 680, r=1.5)
    div = expr(mat, unreal.MaterialExpressionDivide, 50, 520)
    connect(sub, div, "", "A")
    connect(k15, div, "", "B")
    sat = expr(mat, unreal.MaterialExpressionSaturate, 220, 520)
    connect(div, sat)
    foam_mask = expr(mat, unreal.MaterialExpressionMultiply, 400, 580)
    connect(sat, foam_mask, "", "A")
    connect(c_foam, foam_mask, "", "B")

    # 基色：深海蓝 -> 近黑（OceanDarkness），再与浪花白（foamMask）混合
    col_deep = expr(mat, unreal.MaterialExpressionConstant3Vector, -500, 950)
    col_deep.set_editor_property("constant", unreal.LinearColor(0.004, 0.02, 0.045, 1.0))
    col_dark = expr(mat, unreal.MaterialExpressionConstant3Vector, -500, 1100)
    col_dark.set_editor_property("constant", unreal.LinearColor(0.001, 0.004, 0.009, 1.0))
    lerp_col = expr(mat, unreal.MaterialExpressionLinearInterpolate, -150, 1000)
    connect(col_deep, lerp_col, "", "A")
    connect(col_dark, lerp_col, "", "B")
    connect(c_dark, lerp_col, "", "Alpha")
    col_foam = expr(mat, unreal.MaterialExpressionConstant3Vector, 150, 750)
    col_foam.set_editor_property("constant", unreal.LinearColor(0.75, 0.8, 0.82, 1.0))
    lerp_final = expr(mat, unreal.MaterialExpressionLinearInterpolate, 650, 900)
    connect(lerp_col, lerp_final, "", "A")
    connect(col_foam, lerp_final, "", "B")
    connect(foam_mask, lerp_final, "", "Alpha")
    to_prop(lerp_final, unreal.MaterialProperty.MP_BASE_COLOR)

    # 粗糙度：水面 0.06 -> 浪花 0.6
    k_r0 = expr(mat, unreal.MaterialExpressionConstant, 400, 1050, r=0.06)
    k_r1 = expr(mat, unreal.MaterialExpressionConstant, 400, 1100, r=0.6)
    lerp_rough = expr(mat, unreal.MaterialExpressionLinearInterpolate, 650, 1050)
    connect(k_r0, lerp_rough, "", "A")
    connect(k_r1, lerp_rough, "", "B")
    connect(foam_mask, lerp_rough, "", "Alpha")
    to_prop(lerp_rough, unreal.MaterialProperty.MP_ROUGHNESS)

    spec = expr(mat, unreal.MaterialExpressionConstant, 400, 1150, r=0.5)
    to_prop(spec, unreal.MaterialProperty.MP_SPECULAR)

    try:
        MEL.recompile_material(mat)
    except Exception:
        pass
    save(mat)
    return mat


# ---------------------------------------------------------------- 5. 关卡

def load_bt(name):
    return unreal.load_class(None, "/Script/BlackTide." + name)


def setup_level(mpc, profiles, m_ocean):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(PATH_MAP):
        les.load_level(PATH_MAP)
    else:
        les.new_level(PATH_MAP)

    # 幂等：清理本关卡中已有的 BT 演员及环境演员，避免重复生成
    PURGE_CLASSES = ("SkyAtmosphere", "VolumetricCloud", "ExponentialHeightFog",
                     "DirectionalLight", "SkyLight", "PostProcessVolume", "PlayerStart")
    for a in eas.get_all_level_actors():
        try:
            cname = a.get_class().get_name()
            label = a.get_actor_label() or ""
            if cname.startswith("BT") or label.startswith("BT_") or cname in PURGE_CLASSES:
                eas.destroy_actor(a)
        except Exception:
            pass

    V = unreal.Vector
    R = unreal.Rotator

    # --- 核心管理器 ---
    storm = eas.spawn_actor_from_class(load_bt("BTStormManager"), V(0, 0, 0))
    storm.set_actor_label("BT_StormManager")
    storm.set_editor_property("initial_state", unreal.BTWeatherState.STORM)
    storm.set_editor_property("global_mpc", mpc)
    storm.set_editor_property("profiles", {
        unreal.BTWeatherState.CLEAR: profiles["Clear"],
        unreal.BTWeatherState.CLOUDY: profiles["Cloudy"],
        unreal.BTWeatherState.LIGHT_RAIN: profiles["LightRain"],
        unreal.BTWeatherState.HEAVY_RAIN: profiles["HeavyRain"],
        unreal.BTWeatherState.STORM: profiles["Storm"],
        unreal.BTWeatherState.EXTREME_STORM: profiles["ExtremeStorm"],
    })

    om = eas.spawn_actor_from_class(load_bt("BTOceanManager"), V(0, 0, 0))
    om.set_actor_label("BT_OceanManager")

    lc = eas.spawn_actor_from_class(load_bt("BTLightningController"), V(0, 0, 0))
    lc.set_actor_label("BT_LightningController")

    cd = eas.spawn_actor_from_class(load_bt("BTCinematicDirector"), V(0, 0, 0))
    cd.set_actor_label("BT_CinematicDirector")

    # --- 海面（高细分 ProceduralMesh + M_Ocean）---
    ocean = eas.spawn_actor_from_class(load_bt("BTOceanSurfaceActor"), V(0, 0, 0))
    ocean.set_actor_label("BT_OceanSurface")
    ocean.set_editor_property("ocean_material", m_ocean)
    # OnConstruction 在 spawn 时已生成网格；直接设置组件材质覆盖
    # （actor 属性也已保存，关卡重载/重建时会自动套用）
    try:
        pmc = ocean.get_component_by_class(unreal.ProceduralMeshComponent)
        if pmc:
            pmc.set_material(0, m_ocean)
    except Exception as ex:
        REPORT["errors"].append("ocean material: %s" % ex)

    # --- 灯光基调：深蓝/黑为主，暖黄船灯后续挂船上 ---
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 2000), R(pitch=-48.0, yaw=-35.0, roll=0.0))
    sun.set_actor_label("BT_MoonKey")
    sunc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sunc.set_editor_property("intensity", 3.0)
    sunc.set_editor_property("light_color", unreal.Color(r=130, g=160, b=210, a=255))

    sky = eas.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 500))
    sky.set_actor_label("BT_SkyLight")
    skyc = sky.get_component_by_class(unreal.SkyLightComponent)
    skyc.set_editor_property("intensity", 0.8)
    skyc.set_editor_property("light_color", unreal.Color(r=110, g=135, b=180, a=255))

    eas.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0))

    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0))
    fog.set_actor_label("BT_HeightFog")
    fogc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fogc.set_editor_property("fog_density", 0.02)

    eas.spawn_actor_from_class(unreal.VolumetricCloud, V(0, 0, 0))

    pp = eas.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0))
    pp.set_actor_label("BT_PostProcess")
    pp.set_editor_property("unbound", True)

    eas.spawn_actor_from_class(unreal.PlayerStart, V(0, 0, 400))

    # --- GameMode（DefaultEngine.ini 已兜底，这里再设关卡级覆盖）---
    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
        ws = world.get_world_settings()
        if ws:
            ws.set_editor_property("default_game_mode", load_bt("BTGameMode"))
    except Exception as ex:
        REPORT["errors"].append("world gamemode: %s" % ex)

    les.save_current_level()


# ---------------------------------------------------------------- main

def main():
    mpc = setup_mpc()
    step("MPC_Global", True, "8 scalar params")

    profiles = setup_profiles()
    step("StormProfiles", len(profiles) == 6, "%d profiles" % len(profiles))

    master = setup_master()
    step("M_Master", True)
    setup_instances(master)
    step("MaterialInstances", True, "MI_Wood/MI_Metal/MI_Cloth")

    m_ocean = setup_ocean(mpc)
    step("M_Ocean", True, "Gerstner WPO")

    setup_level(mpc, profiles, m_ocean)
    step("Level", True, PATH_MAP)


try:
    main()
except Exception:
    REPORT["errors"].append(traceback.format_exc())

try:
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
except Exception as ex:
    REPORT["errors"].append("save_dirty: %s" % ex)

try:
    with open(REPORT_PATH, "w") as f:
        json.dump(REPORT, f, indent=2, default=str)
except Exception:
    pass

unreal.log("[BT] DONE steps=%d errors=%d" % (len(REPORT["steps"]), len(REPORT["errors"])))
