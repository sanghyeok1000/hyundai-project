import unreal as u,json
from pathlib import Path
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
scene=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='FogHighwayScene')
report={'actor_location':str(scene.get_actor_location()),'components':[]}
for c in scene.get_components_by_class(u.SceneComponent):
 d={'name':c.get_name(),'location':str(c.get_world_location()),'scale':str(c.get_world_scale())}
 if isinstance(c,u.MeshComponent):d['materials']=[str(c.get_material(i)) for i in range(c.get_num_materials())]
 if isinstance(c,u.LocalFogVolumeComponent):d['properties']={k:str(c.get_editor_property(k)) for k in ['radial_fog_extinction','height_fog_extinction','fog_emissive','fog_albedo']}
 report['components'].append(d)
report['cvars']={k:u.SystemLibrary.get_console_variable_int_value(k) for k in ['r.LocalFogVolume','r.SupportLocalFogVolumes','r.Mobile.LocalFogVolume','r.Mobile.DisableVertexFog','r.MobileHDR']}
(Path(u.Paths.project_dir()).resolve().parents[1] / 'docs/research/fog-scene-inspect.json').write_text(json.dumps(report,indent=2))
