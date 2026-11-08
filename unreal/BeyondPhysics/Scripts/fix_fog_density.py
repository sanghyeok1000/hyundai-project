import unreal as u
s=next(a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label()=='FogHighwayScene')
s.set_editor_property('radial_density',.05)
s.get_component_by_class(u.LocalFogVolumeComponent).set_editor_property('height_fog_extinction',1000.)
u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
u.log('FOG_DENSITY_FIXED')
