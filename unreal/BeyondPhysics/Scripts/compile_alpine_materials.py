import unreal as u
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Maps/IceTrackDrive')
m=u.EditorAssetLibrary.load_asset('/Game/Driving/Materials/M_AlpineRockSnow_V2')
u.MaterialEditingLibrary.recompile_material(m)
u.EditorAssetLibrary.save_loaded_asset(m)
u.log('ALPINE_MATERIAL_STATS '+str(u.MaterialEditingLibrary.get_statistics(m)))
