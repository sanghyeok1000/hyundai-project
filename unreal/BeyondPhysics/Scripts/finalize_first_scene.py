"""Correct the initial proxy's wheel orientation and set a useful editor view.

Python's Rotator constructor orders roll, pitch, yaw, unlike FRotator in C++.
Always name these arguments at the language boundary.
"""
import json
from pathlib import Path
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level("/Game/Maps/PhysicsLab"):
    raise RuntimeError("Cannot load PhysicsLab")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
changes = []
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("Car_Wheel"):
        before = str(actor.get_actor_rotation())
        actor.set_actor_rotation(unreal.Rotator(roll=90, pitch=0, yaw=0), False)
        changes.append({"label": label, "before": before, "after": str(actor.get_actor_rotation())})
    elif label == "Sun":
        actor.set_actor_rotation(unreal.Rotator(roll=0, pitch=-48, yaw=-35), False)
location = unreal.Vector(780, -1050, 560)
rotation = unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(0, 0, 70))
unreal.EditorLevelLibrary.set_level_viewport_camera_info(location, rotation)
if not levels.save_current_level():
    raise RuntimeError("Cannot save scene correction")
Path(unreal.Paths.project_saved_dir(), "scene-finalization.json").write_text(json.dumps(changes, indent=2), encoding="utf-8")
unreal.log("BEYOND PHYSICS scene finalized; corrected wheel count: " + str(len(changes)))
