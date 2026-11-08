import unreal as u,json
from pathlib import Path
root=(Path(u.Paths.project_dir()).resolve().parents[1] / 'unreal/BeyondPhysics')
scope={'__name__':'fog_materials'};exec((root/'Scripts/configure_fog_materials.py').read_text(encoding='utf-8-sig'),scope);scope['apply_to_map']()
actors=u.get_editor_subsystem(u.EditorActorSubsystem);ml=u.MaterialEditingLibrary;a=u.EditorAssetLibrary
extended=u.SystemLibrary.get_console_variable_int_value('r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange')
exposure=13. if extended else 1024.
for actor in actors.get_all_level_actors():
 if isinstance(actor,u.SkyLight):actor.get_component_by_class(u.SkyLightComponent).set_intensity(.08)
 if isinstance(actor,u.DirectionalLight):actor.get_component_by_class(u.DirectionalLightComponent).set_light_color(u.LinearColor(.95,.98,1))
 if isinstance(actor,u.PostProcessVolume):
  s=actor.get_editor_property('settings');s.set_editor_property('auto_exposure_min_brightness',exposure);s.set_editor_property('auto_exposure_max_brightness',exposure);actor.set_editor_property('settings',s)
paint=a.load_asset('/Game/Driving/Fog/MI_FogLeadPaint');ml.set_material_instance_vector_parameter_value(paint,'Color',u.LinearColor(.045,.075,.11,1));a.save_loaded_asset(paint)
tail=a.load_asset('/Game/Driving/Fog/MI_FogTailLamp');ml.set_material_instance_scalar_parameter_value(tail,'Glow',150);ml.set_material_instance_vector_parameter_value(tail,'GlowColor',u.LinearColor(1,.01,.003,1));a.save_loaded_asset(tail)
if not u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level():raise RuntimeError('Cannot save final look')
(root.parent.parent/'docs/research/fog-final-look.json').write_text(json.dumps({'extended_luminance_range':extended,'fixed_exposure_value':exposure,'skylight_intensity':.08,'sun_lux':5000,'body_slot':1,'chassis_slot':0,'tail_glow':150,'radial':.27,'height_mask':1000},indent=2))


