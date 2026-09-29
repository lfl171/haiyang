# -*- coding: utf-8 -*-
"""
BLACK TIDE - 海盗船模型导入脚本（幂等）
  1. 导入 Kenney Pirate Kit FBX -> /Game/BLACKTIDE/Ship/Mesh
  2. 创建 BP_PirateShip（父类 ABTShipActor），装配 Hull/Mast 网格
  3. 放入 L_BlackTide_Main，按 Hull 包围盒缩放到约 40m 船长
"""
import json
import traceback
import unreal

REPORT = {"steps": [], "created": [], "errors": []}
REPORT_PATH = r"E:\UE5 CPP\T_2\Scripts\import_ship_report.json"

FBX_DIR = r"D:\BTAssets\pirate-kit\Models\FBX format"
DEST_MESH = "/Game/BLACKTIDE/Ship/Mesh"
DEST_BP = "/Game/BLACKTIDE/Ship/Blueprint"
MAP = "/Game/BLACKTIDE/Maps/L_BlackTide_Main"

# Interchange 将 ship-pirate-large.fbx 拆为多个网格：船体(含桅杆)/sail-a/sail-b/flag-c*3
MESH_NAMES = {"hull": "ship-pirate-large", "sail": "sail-a"}


def step(name, ok, detail=""):
    REPORT["steps"].append({"name": name, "ok": bool(ok), "detail": str(detail)})
    unreal.log("[BT-SHIP] %s => %s %s" % (name, "OK" if ok else "FAIL", detail))


def save(obj):
    unreal.EditorAssetLibrary.save_asset(obj.get_path_name().split(".")[0])


def import_fbx():
    full_hull = DEST_MESH + "/" + MESH_NAMES["hull"]
    if not unreal.EditorAssetLibrary.does_asset_exist(full_hull):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", FBX_DIR + "\\ship-pirate-large.fbx")
        task.set_editor_property("destination_path", DEST_MESH)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        opt = unreal.FbxImportUI()
        opt.set_editor_property("import_mesh", True)
        opt.set_editor_property("import_materials", True)
        opt.set_editor_property("import_textures", True)
        opt.set_editor_property("import_as_skeletal", False)
        task.set_editor_property("options", opt)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        try:
            REPORT["created"] += [str(p) for p in (task.get_editor_property("imported_object_paths") or [])]
        except Exception:
            pass

    out = {}
    for key, name in MESH_NAMES.items():
        p = DEST_MESH + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            out[key] = unreal.EditorAssetLibrary.load_asset(p)
    if "hull" not in out:
        raise RuntimeError("hull mesh missing after import")
    return out


def make_blueprint(meshes):
    full = DEST_BP + "/BP_PirateShip"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        bp = unreal.EditorAssetLibrary.load_asset(full)
    else:
        f = unreal.BlueprintFactory()
        f.set_editor_property("parent_class", unreal.load_class(None, "/Script/BlackTide.BTShipActor"))
        bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "BP_PirateShip", DEST_BP, unreal.Blueprint, f)
        if bp is None:
            raise RuntimeError("create BP_PirateShip failed")
        REPORT["created"].append(full)

    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    hull = cdo.get_editor_property("hull")
    if hull:
        hull.set_editor_property("static_mesh", meshes["hull"])
    sail = cdo.get_editor_property("sail")
    if sail and "sail" in meshes:
        sail.set_editor_property("static_mesh", meshes["sail"])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
    return bp


def place_ship(bp, meshes):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level(MAP)

    for a in eas.get_all_level_actors():
        try:
            if (a.get_actor_label() or "").startswith("BT_PirateShip"):
                eas.destroy_actor(a)
        except Exception:
            pass

    actor = eas.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, 0))
    actor.set_actor_label("BT_PirateShip")

    # 按 Hull 包围盒把船长缩放到约 40 米（4000uu）
    try:
        bb = meshes["hull"].get_bounding_box()
        length = bb.max.x - bb.min.x
        if length > 0.0:
            s = max(0.1, min(100.0, 4000.0 / length))
            actor.set_actor_scale3d(unreal.Vector(s, s, s))
            step("ShipScale", True, "raw_len=%.1f scale=%.3f" % (length, s))
    except Exception as ex:
        REPORT["errors"].append("scale: %s" % ex)

    les.save_current_level()


def main():
    meshes = import_fbx()
    step("ImportFBX", "hull" in meshes, "%d meshes wired" % len(meshes))
    bp = make_blueprint(meshes)
    step("BP_PirateShip", True)
    place_ship(bp, meshes)
    step("PlaceShip", True, MAP)


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

unreal.log("[BT-SHIP] DONE steps=%d errors=%d" % (len(REPORT["steps"]), len(REPORT["errors"])))
