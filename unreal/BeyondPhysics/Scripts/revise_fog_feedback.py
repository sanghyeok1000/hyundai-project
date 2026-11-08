"""Run after Editor build. Only rebuild the fog scene actor and fog map."""
import unreal as u,json
from pathlib import Path
levels=u.get_editor_subsystem(u.LevelEditorSubsystem);actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level('/Game/Maps/FogHighwayDrive'):raise RuntimeError('Cannot open fog map')
for actor in list(actors.get_all_level_actors()):
 if actor.get_actor_label()=='FogHighwayScene':actors.destroy_actor(actor)
 if isinstance(actor,u.PlayerStart):actor.set_actor_location(u.Vector(-26000,0,120),False,False)
scene=actors.spawn_actor_from_class(u.load_class(None,'/Script/BeyondPhysics.FogHighwayScene'),u.Vector(0,0,0));scene.set_actor_label('FogHighwayScene');scene.set_folder_path('FogHighway')
scene.set_editor_property('radial_density',.27);scene.set_editor_property('fog_radius_meters',450.);scene.set_editor_property('fog_center_x_meters',220.)
if not levels.save_current_level():raise RuntimeError('Cannot save fog revision')
report={'map':'/Game/Maps/FogHighwayDrive','start_x_m':-260,'initial_kmh':80,'target_kmh':100,'fog_radius_m':450,'fog_center_x_m':220,'fog_entry_distance_m':30,'radial_density':.27,'reference_gap_m':30,'obstacles':[{'x_m':0,'y_left_m':0},{'x_m':45,'y_left_m':3.5},{'x_m':85,'y_left_m':-3.5}],'components':[]}
for c in scene.get_components_by_class(u.StaticMeshComponent):
 if c.get_name() in ['StationaryLeadBody','Traffic1Body','Traffic2Body']:
  loc=c.get_world_location();report['components'].append({'name':c.get_name(),'location_cm':{'x':loc.x,'y':loc.y,'z':loc.z},'paint':c.get_material(1).get_path_name()})
(Path(u.Paths.project_dir()).resolve().parents[1] / 'docs/research/fog-feedback-scene.json').write_text(json.dumps(report,indent=2))
u.log('FOG_FEEDBACK_SCENE_SAVED')
