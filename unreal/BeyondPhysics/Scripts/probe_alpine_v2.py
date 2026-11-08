import unreal as u, json
from pathlib import Path
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Maps/IceTrackDrive')
result=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if 'AlpineBackdrop' not in a.get_actor_label(): continue
    c=a.static_mesh_component
    result.append({'actor':a.get_actor_label(),'hidden':a.get_editor_property('hidden'),
        'visible':c.get_editor_property('visible'),'mesh':c.static_mesh.get_path_name(),
        'materials':[c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials())]})
Path(u.Paths.project_saved_dir(),'alpine-material-probe.json').write_text(json.dumps(result,indent=2))
u.log('ALPINE_PROBE '+json.dumps(result))
