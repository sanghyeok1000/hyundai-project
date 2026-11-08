"""Project-only instancing materials; preserve every original material and map."""
import unreal as u

def prepare():
    a=u.EditorAssetLibrary;m=u.MaterialEditingLibrary;folder='/Game/Driving/Fog'
    parent_path=folder+'/M_FogSurface'
    parent=a.load_asset(parent_path) if a.does_asset_exist(parent_path) else a.duplicate_asset('/Game/Driving/Materials/M_Surface',parent_path)
    m.set_base_material_usage(parent,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True)
    m.recompile_material(parent);a.save_loaded_asset(parent)
    pairs=[('Road','MI_AsphaltRoad','MI_FogRoad'),('Shoulder','MI_AsphaltShoulder','MI_FogShoulder'),('Markings','MI_TrackPaint','MI_FogPaint'),('Barriers','MI_RoadConcrete','MI_FogConcrete'),('LampSteel','MI_LampSteel','MI_FogLampSteel'),('LampHeads','MI_LampWarm','MI_FogLampWarm'),('Reflectors','MI_LightAmber','MI_FogAmber')]
    mapping={}
    for component,source,dest in pairs:
        path=folder+'/'+dest
        mi=a.load_asset(path) if a.does_asset_exist(path) else a.duplicate_asset('/Game/Driving/Materials/'+source,path)
        m.set_material_instance_parent(mi,parent)
        m.set_material_usage_override(mi,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True,True)
        m.update_material_instance(mi);a.save_loaded_asset(mi);mapping[component]=mi
    for component,name in [('Verge','MI_FogVerge'),('LeadTailLamps','MI_FogTailLamp'),('StationaryLeadBody','MI_FogLeadPaint')]:
        mi=a.load_asset(folder+'/'+name);m.set_material_instance_parent(mi,parent)
        m.set_material_usage_override(mi,u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,True,True)
        m.update_material_instance(mi);a.save_loaded_asset(mi);mapping[component]=mi
    return mapping

def apply_to_map():
    mapping=prepare();levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/FogHighwayDrive'):raise RuntimeError('Fog map missing')
    actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    actor=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='FogHighwayScene')
    for c in actor.get_components_by_class(u.MeshComponent):
        if c.get_name() in mapping:
            c.set_material(1 if c.get_name()=='StationaryLeadBody' else 0,mapping[c.get_name()])
            if c.get_name()=='StationaryLeadBody':c.set_material(0,c.get_editor_property('static_mesh').get_material(0))
    actor.set_editor_property('radial_density',.27)
    actor.set_editor_property('fog_radius_meters',450.)
    actor.set_editor_property('fog_center_x_meters',220.)
    actor.get_component_by_class(u.LocalFogVolumeComponent).set_editor_property('height_fog_extinction',1000.)
    # Property edit may run native construction, so apply overrides afterwards too.
    for c in actor.get_components_by_class(u.MeshComponent):
        if c.get_name() in mapping:
            c.set_material(1 if c.get_name()=='StationaryLeadBody' else 0,mapping[c.get_name()])
            if c.get_name()=='StationaryLeadBody':c.set_material(0,c.get_editor_property('static_mesh').get_material(0))
    if not levels.save_current_level():raise RuntimeError('Cannot save fog materials')
    import json
    from pathlib import Path
    report={c.get_name():{'material':mapping[c.get_name()].get_path_name(),'instanced':u.MaterialEditingLibrary.has_material_usage(mapping[c.get_name()],u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)} for c in actor.get_components_by_class(u.MeshComponent) if c.get_name() in mapping}
    (Path(u.Paths.project_dir()).resolve().parents[1] / 'docs/research/fog-instancing-materials.json').write_text(json.dumps(report,indent=2))
    u.log('FOG_INSTANCING_MATERIALS_SAVED')

if __name__=='__main__':apply_to_map()



