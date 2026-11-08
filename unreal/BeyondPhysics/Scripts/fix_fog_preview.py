import unreal as u
assets=u.EditorAssetLibrary; tools=u.AssetToolsHelpers.get_asset_tools(); ml=u.MaterialEditingLibrary; folder='/Game/Driving/Fog'
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

w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
u.SystemLibrary.execute_console_command(w,'ShowFlag.Fog 1')
u.log('FOG_SKY_SAMPLER_FIXED')
