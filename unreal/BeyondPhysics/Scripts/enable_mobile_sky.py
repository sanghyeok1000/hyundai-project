"""Add Epic's atmosphere sky dome required by the mobile renderer.

The original map is backed up before this script. No vehicle dynamics change.
"""
import json
from pathlib import Path
import unreal as u

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
assets = u.EditorAssetLibrary
if not levels.load_level('/Game/Maps/IceTrackDrive'):
    raise RuntimeError('Unable to load driving map')
mesh = u.load_asset('/Engine/EngineSky/SM_SkySphere')
material = u.load_asset('/Engine/EngineSky/SkyAtmosphere/SkyAtmosphere_MaterialSkyDome')
if not mesh or not material:
    raise RuntimeError('Epic atmosphere sky dome asset is missing')
label = 'Mobile_AtmosphereSkyDome'
sky = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == label), None)
if sky is None:
    sky = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(0, 0, 0))
    sky.set_actor_label(label)
    sky.set_folder_path('Environment')
component = sky.get_editor_property('static_mesh_component')
component.set_static_mesh(mesh)
component.set_material(0, material)
component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
component.set_editor_property('cast_shadow', False)
sky.set_actor_scale3d(u.Vector(100, 100, 100))
if not levels.save_current_level():
    raise RuntimeError('Unable to save mobile sky dome')
report = {'status': 'saved', 'map': '/Game/Maps/IceTrackDrive', 'actor': label,
          'mesh': mesh.get_path_name(), 'material': material.get_path_name(),
          'is_sky': material.get_editor_property('is_sky')}
if not report['is_sky']:
    raise RuntimeError('Sky material lacks the mobile IsSky flag')
(Path(u.Paths.project_saved_dir()) / 'mobile-sky-result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
u.log('MOBILE_SKY_READY ' + json.dumps(report))
