# -*- coding: utf-8 -*-
"""BLACK TIDE - 编辑器自动取景截图（-ExecutePythonScript 启动时运行）"""
import unreal

SHOTS = [
    (800, r"D:\BTAssets\shots\blacktide_01.png"),
    (1600, r"D:\BTAssets\shots\blacktide_02.png"),
    (2400, r"D:\BTAssets\shots\blacktide_03.png"),
]
_state = {"n": 0, "done": set()}

try:
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level("/Game/BLACKTIDE/Maps/L_BlackTide_Main")
    # 相机对准原点处的海盗船（船长 40m）
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(
        unreal.Vector(-2500.0, -1800.0, 900.0),
        unreal.Rotator(pitch=-14.0, yaw=35.7, roll=0.0))
    unreal.log("[BT-SHOT] camera set")
except Exception as ex:
    unreal.log_error("[BT-SHOT] setup failed: %s" % ex)


def _cb(dt):
    _state["n"] += 1
    for frame, path in SHOTS:
        if _state["n"] >= frame and frame not in _state["done"]:
            _state["done"].add(frame)
            try:
                unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, path)
                unreal.log("[BT-SHOT] saved %s" % path)
            except Exception as ex:
                unreal.log_error("[BT-SHOT] shot failed: %s" % ex)


_handle = unreal.register_slate_post_tick_callback(_cb)
