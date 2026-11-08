"""Run with Unreal Editor's Python plugin. Creates assets once; never overwrites a map.

The generated vehicle consists of visual primitives. Nothing here calculates
vehicle dynamics, tire forces, suspension, or Pleos input.
"""
import json
from pathlib import Path
import unreal


MAP = "/Game/Maps/PhysicsLab"
ROOT = "/Game/Phase1"
assets = unreal.EditorAssetLibrary
if assets.does_asset_exist(MAP):
    raise RuntimeError("PhysicsLab already exists. Refusing to overwrite edited work.")
assets.make_directory("/Game/Maps")
assets.make_directory(ROOT + "/Materials")
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(MAP):
    raise RuntimeError("Could not create PhysicsLab level")


def material(name, color, roughness=0.8):
    path = ROOT + "/Materials/" + name
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew()
    )
    base = unreal.MaterialEditingLibrary.create_material_expression(
        result, unreal.MaterialExpressionConstant3Vector, -300, 0
    )
    base.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        result, unreal.MaterialExpressionConstant, -300, 180
    )
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(result)
    assets.save_loaded_asset(result)
    return result


road_mat = material("M_Asphalt", (0.045, 0.055, 0.065))
ground_mat = material("M_Ground", (0.11, 0.15, 0.14))
paint_mat = material("M_White", (0.75, 0.78, 0.76))
body_mat = material("M_Body", (0.045, 0.24, 0.42), 0.35)
glass_mat = material("M_GlassOpaque", (0.015, 0.035, 0.055), 0.2)
tire_mat = material("M_Tire", (0.012, 0.014, 0.016))
light_mat = material("M_Headlight", (0.8, 0.86, 0.7), 0.3)


def shape(label, kind, location, scale, surface, rotation=None):
    obj = actors.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*location), rotation or unreal.Rotator()
    )
    obj.set_actor_label(label)
    obj.set_folder_path("Vehicle" if label.startswith("Car_") else "Environment")
    component = obj.get_editor_property("static_mesh_component")
    mesh = assets.load_asset("/Engine/BasicShapes/" + kind + "." + kind)
    if mesh is None or not component.set_static_mesh(mesh):
        raise RuntimeError("Could not assign mesh: " + label)
    component.set_material(0, surface)
    component.set_simulate_physics(False)
    obj.set_actor_scale3d(unreal.Vector(*scale))
    return obj


# Unreal world distances are centimetres. A basic Cube has a 100 cm side.
shape("Ground_200m", "Cube", (0, 0, -16), (200, 200, 0.2), ground_mat)
shape("Road_200m", "Cube", (0, 0, -5), (200, 14, 0.1), road_mat)
for side in (-1, 1):
    shape("Road_Edge_" + str(side), "Cube", (0, side * 620, 0.5), (200, 0.12, 0.01), paint_mat)
    for i, x in enumerate(range(-9600, 9601, 800)):
        shape("Lane_{}_{}".format(side, i), "Cube", (x, side * 180, 0.5), (4, 0.12, 0.01), paint_mat)

# 4.4 m visual proxy only. These dimensions are not measured Hyundai parameters.
shape("Car_Body", "Cube", (0, 0, 70), (4.4, 1.8, 0.55), body_mat)
shape("Car_Cabin", "Cube", (-25, 0, 115), (2.3, 1.5, 0.4), glass_mat)
shape("Car_Roof", "Cube", (-30, 0, 139), (2.1, 1.55, 0.08), body_mat)
for x in (-140, 140):
    for y in (-94, 94):
        shape("Car_Wheel_{}_{}".format(x, y), "Cylinder", (x, y, 33.5),
              (0.66, 0.66, 0.24), tire_mat, unreal.Rotator(roll=90, pitch=0, yaw=0))
for side in (-1, 1):
    shape("Car_Headlight_" + str(side), "Cube", (221, side * 58, 76),
          (0.025, 0.35, 0.14), light_mat)

sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(roll=0, pitch=-48, yaw=-35))
sun.set_actor_label("Sun")
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun_component.set_intensity(3.0)
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
sky.set_actor_label("SkyFill")
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_component.set_intensity(1.0)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())

camera_location = unreal.Vector(780, -1050, 560)
camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, unreal.Vector(0, 0, 70))
camera = actors.spawn_actor_from_class(unreal.CameraActor, camera_location, camera_rotation)
camera.set_actor_label("LabCamera")
camera.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
camera.get_component_by_class(unreal.CameraComponent).set_editor_property("field_of_view", 55.0)

if not levels.save_current_level():
    raise RuntimeError("Could not save PhysicsLab")
assets.save_directory("/Game")
report = {
    "map": MAP,
    "actors": len(actors.get_all_level_actors()),
    "vehicle_dynamics": "not implemented",
    "world_distance_unit": "centimetre",
    "camera": "LabCamera",
    "status": "scene_created"
}
report_path = Path(unreal.Paths.project_saved_dir()) / "first-scene-result.json"
report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("BEYOND PHYSICS first scene saved: " + MAP)
