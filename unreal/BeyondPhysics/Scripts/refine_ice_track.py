import unreal as u, json
from pathlib import Path
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level('/Game/Maps/IceTrackDrive')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
report={}
for a in actors.get_all_level_actors():
    if a.get_actor_label()=='AlpineBackdrop':
        b=a.static_mesh_component.static_mesh.get_bounding_box()
        size=b.max-b.min
        report['mountain_bounds']=str(size)
        if size.z>size.y: a.set_actor_rotation(u.Rotator(roll=90,pitch=0,yaw=0),False)
        if size.x>1000000: a.set_actor_scale3d(u.Vector(.01,.01,.01))
    if a.get_actor_label()=='Look_ColdIce_WarmSun':
        s=a.settings; s.auto_exposure_min_brightness=8; s.auto_exposure_max_brightness=8; a.settings=s
levels.save_current_level()
Path(u.Paths.project_saved_dir(),'ice-track-refinement.json').write_text(json.dumps(report))
