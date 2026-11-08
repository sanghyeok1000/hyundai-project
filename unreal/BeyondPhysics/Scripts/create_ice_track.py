"""Create the editable IceTrack art and level. Never overwrite an existing map.
Run with UnrealEditor-Cmd -ExecutePythonScript=<this file>.
All distances in the generated scene are cm; this script is editor-only.
"""
import json, math, random, struct
from pathlib import Path
import unreal as u

MAP='/Game/Maps/IceTrackDrive'
ROOT='/Game/Driving'
assets=u.EditorAssetLibrary
retry='-IceTrackRetry' in u.SystemLibrary.get_command_line()
if assets.does_asset_exist(MAP) and not retry:
    raise RuntimeError('IceTrack already exists. Refusing to overwrite your edited level.')
project=Path(u.Paths.project_dir())
art=project/'SourceArt'/'IceTrack'
art.mkdir(parents=True,exist_ok=True)
for folder in ('Materials','Textures','Meshes'):
    assets.make_directory(ROOT+'/'+folder)

# Deterministic tileable Voronoi fracture texture, generated from equations.
# These are visual textures; they do not set physical tire friction.
N=512
cells=8
rng=random.Random(41028)
points={(x,y):(x+rng.random(),y+rng.random()) for x in range(cells) for y in range(cells)}
heights=[]; colors=[]
for y in range(N):
    for x in range(N):
        px=x/N*cells; py=y/N*cells; gx=int(px); gy=int(py)
        distances=[]
        for dx in (-1,0,1):
            for dy in (-1,0,1):
                cx=gx+dx; cy=gy+dy
                ox,oy=points[(cx%cells,cy%cells)]
                ox+=cx-cx%cells; oy+=cy-cy%cells
                distances.append((px-ox)**2+(py-oy)**2)
        distances.sort()
        edge=math.sqrt(distances[1])-math.sqrt(distances[0])
        crack=math.exp(-edge*edge/0.00011)
        cloud=(math.sin(px*4.4+math.sin(py*2.3))+math.sin(py*5.1+px*1.9))*0.5
        grain=((x*73856093 ^ y*19349663)&255)/255-.5
        brightness=.70+.08*cloud+.03*grain
        colors.append((int(88*brightness+94*crack),int(122*brightness+88*crack),int(143*brightness+72*crack)))
        heights.append(.003*cloud+.001*grain-.012*crack)

def tga(name,pixels):
    data=bytearray(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,N,N,24,0x20))
    for r,g,b in pixels: data.extend((max(0,min(255,b)),max(0,min(255,g)),max(0,min(255,r))))
    path=art/(name+'.tga'); path.write_bytes(data); return path

normal=[]
for y in range(N):
    for x in range(N):
        dx=(heights[y*N+(x+1)%N]-heights[y*N+(x-1)%N])*18
        dy=(heights[((y+1)%N)*N+x]-heights[((y-1)%N)*N+x])*18
        length=math.sqrt(dx*dx+dy*dy+1)
        normal.append((int(127.5-dx/length*127.5),int(127.5-dy/length*127.5),int(127.5+127.5/length)))

def import_file(path,destination,name):
    if assets.does_asset_exist(destination+'/'+name): return assets.load_asset(destination+'/'+name)
    task=u.AssetImportTask()
    task.filename=str(path); task.destination_path=destination; task.destination_name=name
    task.automated=True; task.replace_existing=False; task.save=True
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    result=assets.load_asset(destination+'/'+name)
    if not result: raise RuntimeError('Import failed: '+str(path))
    return result

albedo=import_file(tga('T_IceColor',colors),ROOT+'/Textures','T_IceColor')
normal_map=import_file(tga('T_IceNormal',normal),ROOT+'/Textures','T_IceNormal')
normal_map.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
normal_map.set_editor_property('srgb',False)
assets.save_loaded_asset(normal_map)

tools=u.AssetToolsHelpers.get_asset_tools()
matlib=u.MaterialEditingLibrary
def material(name):
    if assets.does_asset_exist(ROOT+'/Materials/'+name): return assets.load_asset(ROOT+'/Materials/'+name)
    return tools.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
def node(mat,cls,**props):
    n=matlib.create_material_expression(mat,cls,0,0)
    for k,v in props.items(): n.set_editor_property(k,v)
    return n
def link(a,b,pin,output=''): matlib.connect_material_expressions(a,output,b,pin)
def scalar(mat,name,value):
    return node(mat,u.MaterialExpressionScalarParameter,parameter_name=name,default_value=value,group='Surface')
def vector(mat,name,rgb):
    return node(mat,u.MaterialExpressionVectorParameter,parameter_name=name,default_value=u.LinearColor(*rgb,1),group='Surface')
def out(n,mat,prop): matlib.connect_material_property(n,'',prop)
def finish(mat):
    matlib.layout_material_expressions(mat); matlib.recompile_material(mat); assets.save_loaded_asset(mat)
def instance(name,parent,scalars=None,vectors=None):
    m=assets.load_asset(ROOT+'/Materials/'+name) if assets.does_asset_exist(ROOT+'/Materials/'+name) else tools.create_asset(name,ROOT+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    matlib.set_material_instance_parent(m,parent)
    for k,v in (scalars or {}).items(): matlib.set_material_instance_scalar_parameter_value(m,k,v)
    for k,v in (vectors or {}).items(): matlib.set_material_instance_vector_parameter_value(m,k,u.LinearColor(*v,1))
    assets.save_loaded_asset(m); return m

road=material('M_TestSurface')
world=node(road,u.MaterialExpressionWorldPosition)
mask=node(road,u.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False); link(world,mask,'Input')
tiling=scalar(road,'WorldTiling',1/900)
uv=node(road,u.MaterialExpressionMultiply); link(mask,uv,'A'); link(tiling,uv,'B')
tex=node(road,u.MaterialExpressionTextureSample,texture=albedo); link(uv,tex,'Coordinates')
normal_tex=node(road,u.MaterialExpressionTextureSample,texture=normal_map,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL); link(uv,normal_tex,'Coordinates')
ice=scalar(road,'IceBlend',1)
color=node(road,u.MaterialExpressionMultiply); link(tex,color,'A','RGB'); link(vector(road,'IceTint',(.8,1.05,1.2)),color,'B')
blend=node(road,u.MaterialExpressionLinearInterpolate); link(vector(road,'DryTint',(.035,.045,.055)),blend,'A'); link(color,blend,'B'); link(ice,blend,'Alpha')
out(blend,road,u.MaterialProperty.MP_BASE_COLOR)
rough=node(road,u.MaterialExpressionLinearInterpolate); link(scalar(road,'DryRoughness',.84),rough,'A'); link(scalar(road,'IceRoughness',.21),rough,'B'); link(ice,rough,'Alpha')
out(rough,road,u.MaterialProperty.MP_ROUGHNESS)
out(normal_tex,road,u.MaterialProperty.MP_NORMAL)
out(scalar(road,'Specular',.7),road,u.MaterialProperty.MP_SPECULAR)
finish(road); road_mi=instance('MI_TestSurface',road)

plain=material('M_Surface')
out(vector(plain,'Color',(.5,.6,.7)),plain,u.MaterialProperty.MP_BASE_COLOR)
out(scalar(plain,'Roughness',.8),plain,u.MaterialProperty.MP_ROUGHNESS)
em=node(plain,u.MaterialExpressionMultiply); link(vector(plain,'GlowColor',(0,0,0)),em,'A'); link(scalar(plain,'Glow',0),em,'B'); out(em,plain,u.MaterialProperty.MP_EMISSIVE_COLOR)
finish(plain)
snow=instance('MI_Snow',plain,{'Roughness':.91},{'Color':(.65,.76,.86)})
paint=instance('MI_TrackPaint',plain,{'Roughness':.54},{'Color':(.7,.77,.78)})
dark=instance('MI_Charcoal',plain,{'Roughness':.58},{'Color':(.018,.029,.039)})
cyan=instance('MI_LightCyan',plain,{'Glow':6},{'Color':(.025,.22,.3),'GlowColor':(.04,.65,1)})
amber=instance('MI_LightAmber',plain,{'Glow':5},{'Color':(.4,.17,.03),'GlowColor':(1,.32,.045)})

mountain_mat=material('M_Alpine')
norm=node(mountain_mat,u.MaterialExpressionVertexNormalWS)
up=node(mountain_mat,u.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False); link(norm,up,'Input')
power=node(mountain_mat,u.MaterialExpressionPower,const_exponent=5); link(up,power,'Base')
mountain_color=node(mountain_mat,u.MaterialExpressionLinearInterpolate)
link(vector(mountain_mat,'Rock',(.055,.085,.12)),mountain_color,'A')
link(vector(mountain_mat,'Snow',(.62,.71,.8)),mountain_color,'B'); link(power,mountain_color,'Alpha')
out(mountain_color,mountain_mat,u.MaterialProperty.MP_BASE_COLOR); out(scalar(mountain_mat,'Roughness',.92),mountain_mat,u.MaterialProperty.MP_ROUGHNESS)
finish(mountain_mat)

# A single continuous mountain mesh with smooth vertex normals, no physics.
GRID=101; SIZE=6000
def height(x,y):
    r=max(abs(x)/650,abs(y)/430)
    fade=max(0,min(1,(r-1)*.9))
    ridges=abs(math.sin(x*.0019+y*.0011)+.5*math.cos(x*.0031-y*.0017))
    peaks=180+330*ridges+100*math.sin(x*.006)*math.cos(y*.004)
    return max(0,fade*peaks)
verts=[]; normals=[]
for j in range(GRID):
    for i in range(GRID):
        x=(i/(GRID-1)-.5)*SIZE; y=(j/(GRID-1)-.5)*SIZE
        z=height(x,y)-3
        dx=(height(x+2,y)-height(x-2,y))/4; dy=(height(x,y+2)-height(x,y-2))/4
        inv=1/math.sqrt(dx*dx+dy*dy+1)
        verts.append((x*100,y*100,z*100)); normals.append((-dx*inv,-dy*inv,inv))
lines=['o AlpineBackdrop']
lines+=['v %.4f %.4f %.4f'%v for v in verts]
lines+=['vn %.6f %.6f %.6f'%n for n in normals]
for j in range(GRID-1):
    for i in range(GRID-1):
        a=j*GRID+i+1; b=a+1; c=a+GRID; d=c+1
        lines.append('f %d//%d %d//%d %d//%d'%(a,a,b,b,d,d))
        lines.append('f %d//%d %d//%d %d//%d'%(a,a,d,d,c,c))
obj_path=art/'AlpineBackdrop.obj'; obj_path.write_text('\n'.join(lines),encoding='utf-8')
mountain=import_file(obj_path,ROOT+'/Meshes','SM_AlpineBackdrop')
mountain.set_material(0,mountain_mat); assets.save_loaded_asset(mountain)

levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.new_level(MAP): raise RuntimeError('Could not create IceTrack')
def mesh_actor(name,mesh,location,scale=(1,1,1),mat=None,rotation=None,folder='Track'):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*location),rotation or u.Rotator())
    a.set_actor_label(name); a.set_folder_path(folder)
    c=a.static_mesh_component; c.set_static_mesh(mesh); c.set_simulate_physics(False)
    c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    if mat: c.set_material(0,mat)
    a.set_actor_scale3d(u.Vector(*scale)); return a
cube=assets.load_asset('/Engine/BasicShapes/Cube')
cylinder=assets.load_asset('/Engine/BasicShapes/Cylinder')
surface=mesh_actor('TestSurface_1000x600m',cube,(0,0,-8),(1000,600,.16),road_mi)
surface.tags=['DrivingSurface']
mesh_actor('AlpineBackdrop',mountain,(0,0,0),mat=mountain_mat,folder='Environment')
# Snow starts beyond the planar driving area's limits.
for side in (-1,1):
    mesh_actor('SnowBank_'+str(side),cube,(0,side*35500,-100),(1150,110,2),snow,folder='Environment')
    for x in range(-46000,46001,4000):
        mesh_actor('RunwayLightBase_%d_%d'%(side,x),cube,(x,side*1850,6),(.6,.34,.12),dark)
        mesh_actor('RunwayLight_%d_%d'%(side,x),cube,(x,side*1850,13),(.48,.18,.025),cyan)
    mesh_actor('RunwayEdge_'+str(side),cube,(0,side*1600,1),(950,.12,.015),paint)
for x in range(-46000,46001,1600):
    mesh_actor('CenterDash_'+str(x),cube,(x,0,1.2),(5,.14,.015),paint)
for i in range(11):
    x=-22000+i*2500
    mesh_actor('BrakeDistanceMark_%03dm'%(i*25),cube,(x,1800,1.5),(.15,4,.018),amber if i%2==0 else paint)
    text=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(x,2450,3),u.Rotator(pitch=90,yaw=180,roll=0))
    text.set_actor_label('Distance_%03dm'%(i*25)); text.set_folder_path('Track/Distance')
    tc=text.get_component_by_class(u.TextRenderComponent)
    tc.set_text(str(i*25)+' m'); tc.set_world_size(180); tc.set_text_render_color(u.Color(150,196,208,255))
# Circular skidpad markings: 60 m radius, flat, no hidden lateral constraints.
for k in range(120):
    angle=k*math.tau/120
    mesh_actor('SkidCircle_%03d'%k,cube,(16000+6000*math.cos(angle),-13000+6000*math.sin(angle),1),
               (1.8,.12,.015),paint,u.Rotator(yaw=math.degrees(angle)+90))

sun=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,5000),u.Rotator(pitch=-9,yaw=-32,roll=0))
sun.set_actor_label('Sun_GoldenHour'); sun.set_folder_path('Lighting')
sc=sun.get_component_by_class(u.DirectionalLightComponent); sc.set_mobility(u.ComponentMobility.MOVABLE)
sc.set_intensity(800); sc.set_editor_property('atmosphere_sun_light',True)
sc.set_editor_property('light_source_angle',1.2); sc.set_editor_property('dynamic_shadow_distance_movable_light',18000)
sky=actors.spawn_actor_from_class(u.SkyAtmosphere,u.Vector(0,0,0)); sky.set_actor_label('Sky_Alpine'); sky.set_folder_path('Lighting')
fill=actors.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,3000)); fill.set_actor_label('SkyFill'); fill.set_folder_path('Lighting')
fc=fill.get_component_by_class(u.SkyLightComponent); fc.set_mobility(u.ComponentMobility.MOVABLE); fc.set_intensity(1.1)
fc.set_editor_property('real_time_capture',True)
fog=actors.spawn_actor_from_class(u.ExponentialHeightFog,u.Vector(0,0,-500)); fog.set_actor_label('Haze_Thin'); fog.set_folder_path('Lighting')
fogc=fog.get_component_by_class(u.ExponentialHeightFogComponent); fogc.set_editor_property('fog_density',.000015)
fogc.set_editor_property('start_distance',3000)
pp=actors.spawn_actor_from_class(u.PostProcessVolume,u.Vector()); pp.set_actor_label('Look_ColdIce_WarmSun'); pp.set_folder_path('Lighting')
pp.set_editor_property('unbound',True)
settings=pp.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_method',True)
settings.set_editor_property('auto_exposure_method',u.AutoExposureMethod.AEM_HISTOGRAM)
settings.set_editor_property('override_auto_exposure_min_brightness',True)
settings.set_editor_property('override_auto_exposure_max_brightness',True)
settings.set_editor_property('auto_exposure_min_brightness',7.0)
settings.set_editor_property('auto_exposure_max_brightness',7.0)
settings.set_editor_property('override_auto_exposure_bias',True); settings.set_editor_property('auto_exposure_bias',0)
settings.set_editor_property('override_bloom_intensity',True); settings.set_editor_property('bloom_intensity',.2)
settings.set_editor_property('override_vignette_intensity',True); settings.set_editor_property('vignette_intensity',.22)
settings.set_editor_property('override_motion_blur_amount',True); settings.set_editor_property('motion_blur_amount',0)
pp.set_editor_property('settings',settings)
reflection=actors.spawn_actor_from_class(u.SphereReflectionCapture,u.Vector(0,0,1000)); reflection.set_actor_label('Reflection_Ice'); reflection.set_folder_path('Lighting')
reflection.get_component_by_class(u.SphereReflectionCaptureComponent).set_editor_property('influence_radius',100000)
start=actors.spawn_actor_from_class(u.PlayerStart,u.Vector(-22000,0,100),u.Rotator()); start.set_actor_label('Start_ZeroSpeed')
game_mode=u.load_class(None,'/Script/BeyondPhysics.DrivingGameMode')
if not game_mode: raise RuntimeError('DrivingGameMode missing: build C++ module first')
u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_world_settings().set_editor_property('default_game_mode',game_mode)
if not levels.save_current_level(): raise RuntimeError('Level save failed')
assets.save_directory(ROOT)
report={'map':MAP,'actor_count':len(actors.get_all_level_actors()),'visual_car':'Epic SportsCar assets',
        'physics':'separate C++ planar force model','surface_mu':{'dry':.85,'ice':.12},'art_seed':41028}
Path(u.Paths.project_saved_dir(),'ice-track-result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('BEYOND PHYSICS IceTrack generated successfully')
