import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
levels.load_level('/Game/Maps/IceTrackDrive')
assets=u.EditorAssetLibrary
ml=u.MaterialEditingLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
path='/Game/Driving/Materials/MI_AlpineRockSnow_V2'
mi=assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('MI_AlpineRockSnow_V2','/Game/Driving/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
ml.set_material_instance_parent(mi,assets.load_asset('/Game/Driving/Materials/M_AlpineRockSnow_V2'))
ml.set_material_instance_vector_parameter_value(mi,'RockTint',u.LinearColor(.22,.37,.56,1))
ml.set_material_instance_vector_parameter_value(mi,'SnowColor',u.LinearColor(.50,.58,.64,1))
assets.save_loaded_asset(mi)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if isinstance(a,u.PostProcessVolume):
        s=a.settings; s.auto_exposure_bias=-1.25; a.settings=s
    if a.get_actor_label()=='AlpineBackdrop_Detailed': a.static_mesh_component.set_material(0,mi)
    if isinstance(a,u.DirectionalLight):
        c=a.get_component_by_class(u.DirectionalLightComponent)
        c.set_light_color(u.LinearColor(.92,.96,1))
        a.set_actor_rotation(u.Rotator(pitch=-24,yaw=-45),False)
    if a.get_actor_label()=='Reflection_Ice':
        actors.destroy_actor(a) # Obsolete ice-field capture; full level backup preserves it.
levels.save_current_level()
u.log('ALPINE_LOOK_FINALIZED')

