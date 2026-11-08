"""Run in the freshly built editor. Save a new map copy only; old maps stay unchanged."""
import unreal as u
from pathlib import Path
import json

def build():
    root=Path(u.Paths.project_dir()); assets=u.EditorAssetLibrary; tools=u.AssetToolsHelpers.get_asset_tools(); ml=u.MaterialEditingLibrary
    levels=u.get_editor_subsystem(u.LevelEditorSubsystem); actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    target='/Game/Maps/FogHighwayDrive'; source='/Game/Maps/IceTrackDrive'; folder='/Game/Driving/Fog'
    refresh=bool(globals().get('FOG_REFRESH',False))
    if assets.does_asset_exist(target) and not refresh: raise RuntimeError('Fog map exists; refusing overwrite')
    if not levels.load_level(target if refresh else source):raise RuntimeError('Cannot load source map')
    assets.make_directory(folder)
    def instance(name,color,rough=.8,glow=0,glow_color=(0,0,0)):
        path=folder+'/'+name
        mi=assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(name,folder,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
        ml.set_material_instance_parent(mi,assets.load_asset('/Game/Driving/Materials/M_Surface'))
        ml.set_material_instance_scalar_parameter_value(mi,'Roughness',rough);ml.set_material_instance_scalar_parameter_value(mi,'Glow',glow)
        ml.set_material_instance_vector_parameter_value(mi,'Color',u.LinearColor(*color,1))
        ml.set_material_instance_vector_parameter_value(mi,'GlowColor',u.LinearColor(*glow_color,1));assets.save_loaded_asset(mi)
    instance('MI_FogVerge',(.018,.035,.026),.98)
    instance('MI_FogLeadPaint',(.045,.075,.11),.32)
    instance('MI_FogTailLamp',(.35,.008,.003),.25,150,(1,.01,.003))
    material_scope={'__name__':'fog_materials'}
    exec((root/'Scripts/configure_fog_materials.py').read_text(encoding='utf-8-sig'),material_scope)
    fog_materials=material_scope['prepare']()
    sky_path=folder+'/M_FogCloudySky'
    if True:
        # No existing project material is an opaque two-sided unlit cloudy sky.
        sky=assets.load_asset(sky_path) if assets.does_asset_exist(sky_path) else tools.create_asset('M_FogCloudySky',folder,u.Material,u.MaterialFactoryNew())
        ml.delete_all_material_expressions(sky)
        sky.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT);sky.set_editor_property('two_sided',True);sky.set_editor_property('is_sky',True)
        cloud=ml.create_material_expression(sky,u.MaterialExpressionTextureSample,0,0)
        texture=assets.load_asset('/Engine/EngineSky/T_Sky_Clouds_M');cloud.set_editor_property('texture',texture)
        cloud.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_GRAYSCALE)
        coord=ml.create_material_expression(sky,u.MaterialExpressionTextureCoordinate,-200,0)
        coord.set_editor_property('u_tiling',3);coord.set_editor_property('v_tiling',2)
        ml.connect_material_expressions(coord,'',cloud,'Coordinates')
        colors=[]
        for name,color in [('CloudLight',(1800,1900,2100)),('CloudDark',(900,1050,1250))]:
            node=ml.create_material_expression(sky,u.MaterialExpressionVectorParameter,0,0);node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',u.LinearColor(*color,1));colors.append(node)
        blend=ml.create_material_expression(sky,u.MaterialExpressionLinearInterpolate,0,0)
        ml.connect_material_expressions(colors[0],'',blend,'A');ml.connect_material_expressions(colors[1],'',blend,'B');ml.connect_material_expressions(cloud,'R',blend,'Alpha')
        ml.connect_material_property(blend,'',u.MaterialProperty.MP_EMISSIVE_COLOR);ml.layout_material_expressions(sky);ml.recompile_material(sky);assets.save_loaded_asset(sky)
    baseline=[];hidden=0;sun_count=0;sky_count=0
    for actor in list(actors.get_all_level_actors()):
        if actor.get_actor_label()=='FogHighwayScene':actors.destroy_actor(actor);continue
        if isinstance(actor,u.WorldSettings):continue
        if isinstance(actor,u.PlayerStart):actor.set_actor_location(u.Vector(-26000,0,120),False,False);continue
        if isinstance(actor,u.DirectionalLight):
            component=actor.get_component_by_class(u.DirectionalLightComponent)
            baseline.append({'actor':actor.get_actor_label(),'lux':component.get_editor_property('intensity')})
            component.set_mobility(u.ComponentMobility.MOVABLE);component.set_intensity(5000)
            component.set_light_color(u.LinearColor(.95,.98,1))
            component.set_editor_property('light_source_angle',10);component.set_editor_property('light_source_soft_angle',45)
            actor.set_actor_rotation(u.Rotator(pitch=-35,yaw=-20),False);sun_count+=1;continue
        if isinstance(actor,u.SkyLight):
            component=actor.get_component_by_class(u.SkyLightComponent);component.set_intensity(.08)
            sky_count+=1;continue
        if isinstance(actor,u.PostProcessVolume):
            settings=actor.get_editor_property('settings')
            baseline.append({'actor':actor.get_actor_label(),'min':settings.get_editor_property('auto_exposure_min_brightness'),'max':settings.get_editor_property('auto_exposure_max_brightness'),'bias':settings.get_editor_property('auto_exposure_bias')})
            extended=u.SystemLibrary.get_console_variable_int_value('r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange')
            value=13. if extended else 1024.
            for name,v in [('override_auto_exposure_min_brightness',True),('override_auto_exposure_max_brightness',True),('auto_exposure_min_brightness',value),('auto_exposure_max_brightness',value),('override_auto_exposure_bias',True),('auto_exposure_bias',0.),('override_vignette_intensity',True),('vignette_intensity',.08),('override_bloom_intensity',True),('bloom_intensity',.15)]:settings.set_editor_property(name,v)
            actor.set_editor_property('settings',settings);continue
        actor.set_actor_hidden_in_game(True);actor.set_is_temporarily_hidden_in_editor(True)
        for component in actor.get_components_by_class(u.SceneComponent):component.set_visibility(False);component.set_hidden_in_game(True)
        hidden+=1
    cls=u.load_class(None,'/Script/BeyondPhysics.FogHighwayScene')
    if not cls:raise RuntimeError('Build C++ FogHighwayScene before creating map')
    scene=actors.spawn_actor_from_class(cls,u.Vector(0,0,0));scene.set_actor_label('FogHighwayScene');scene.set_folder_path('FogHighway')
    scene.set_editor_property('radial_density',.27)
    scene.get_component_by_class(u.LocalFogVolumeComponent).set_editor_property('height_fog_extinction',1000.)
    for component in scene.get_components_by_class(u.MeshComponent):
        if component.get_name() in fog_materials:component.set_material(1 if component.get_name()=='StationaryLeadBody' else 0,fog_materials[component.get_name()])
    for actor in actors.get_all_level_actors():
        if isinstance(actor,u.SkyLight):actor.get_component_by_class(u.SkyLightComponent).recapture_sky()
    # SaveMap writes the destination copy. Do NOT call save_current_level on the source world.
    world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if not u.EditorLoadingAndSavingUtils.save_map(world,target):raise RuntimeError('Cannot save fog map copy')
    assets.save_directory(folder)
    report={'map':target,'road_length_m':4000,'lanes':3,'width_m':13.5,'mu':.8,'target_kmh':100,'lead_x_m':0,'hidden_source_actors':hidden,'sun_count':sun_count,'sky_count':sky_count,'lighting_before':baseline,'fog_radius_m':450,'fog_center_x_m':220,'radial_density':.27,'height_mask_density':1000,'friction_source':'NHTSA DOT HS809561 section3.3.8, sliding0.8 / peak0.9','visibility_reference':'30m bumper gap; game measurement, not human detection'}
    (root.parent.parent/'docs/research/fog-art.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    u.log('FOG_HIGHWAY_CREATED '+json.dumps(report))

build()




