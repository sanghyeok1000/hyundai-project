"""Offline Unreal editor art pass. Run only with the interactive editor closed.
Preserves old terrain asset and actor for recovery. Units: cm.
Then run finalize_alpine_look.py for the current material instance and final sun.
"""
import unreal as u
from pathlib import Path
import math, json

ROOT='/Game/Driving'
MAP='/Game/Maps/IceTrackDrive'
project=Path(u.Paths.project_dir())
art=project/'SourceArt'/'AlpineV2'
assets=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
ml=u.MaterialEditingLibrary
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if not levels.load_level(MAP): raise RuntimeError('Cannot load map')
by_label={a.get_actor_label():a for a in actors.get_all_level_actors()}
for folder in ['Meshes','Materials','Textures']:
    assets.make_directory(ROOT+'/'+folder)

def imported(filename,name):
    path=ROOT+'/Textures/'+name
    if assets.does_asset_exist(path): return assets.load_asset(path)
    task=u.AssetImportTask()
    task.filename=str(art/filename); task.destination_path=ROOT+'/Textures'
    task.destination_name=name; task.automated=True; task.save=True
    tools.import_asset_tasks([task])
    result=assets.load_asset(path)
    if not result: raise RuntimeError('Import failed '+path)
    return result
diffuse=imported('rock_diffuse.jpg','T_AlpineRock_Color')
normal=imported('rock_nor_dx.jpg','T_AlpineRock_Normal')
rough=imported('rock_rough.jpg','T_AlpineRock_Roughness')
normal.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
normal.set_editor_property('srgb',False)
rough.set_editor_property('srgb',False)
assets.save_loaded_asset(normal); assets.save_loaded_asset(rough)

def material(name):
    path=ROOT+'/Materials/'+name
    return assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(name,ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
def node(mat,cls,**props):
    obj=ml.create_material_expression(mat,cls,0,0)
    for k,v in props.items(): obj.set_editor_property(k,v)
    return obj
def link(a,b,pin,output=''): 
    # UE's graph shortens the texture Coordinates input label to UVs.
    pin='UVs' if pin=='Coordinates' else pin
    if not ml.connect_material_expressions(a,output,b,pin):
        if pin=='Input' and ml.connect_material_expressions(a,output,b,''): return
        raise RuntimeError('Material link '+pin)
def scalar(mat,name,value): return node(mat,u.MaterialExpressionScalarParameter,parameter_name=name,default_value=value)
def vector(mat,name,value): return node(mat,u.MaterialExpressionVectorParameter,parameter_name=name,default_value=u.LinearColor(*value,1))
def out(n,m,p): ml.connect_material_property(n,'',p)
def finish(m): ml.layout_material_expressions(m); ml.recompile_material(m); assets.save_loaded_asset(m)
def instance(name,color,roughness=.8,glow=0,glowcolor=(0,0,0)):
    path=ROOT+'/Materials/'+name
    mi=assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(name,ROOT+'/Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    ml.set_material_instance_parent(mi,assets.load_asset(ROOT+'/Materials/M_Surface'))
    for k,v in {'Roughness':roughness,'Glow':glow}.items(): ml.set_material_instance_scalar_parameter_value(mi,k,v)
    for k,v in {'Color':color,'GlowColor':glowcolor}.items(): ml.set_material_instance_vector_parameter_value(mi,k,u.LinearColor(*v,1))
    assets.save_loaded_asset(mi); return mi

mountain_mat=material('M_AlpineRockSnow_V2')
# Rebuild only our new material if rerun; old M_Alpine stays intact.
ml.delete_all_material_expressions(mountain_mat)
uv=node(mountain_mat,u.MaterialExpressionTextureCoordinate)
rock=node(mountain_mat,u.MaterialExpressionTextureSample,texture=diffuse)
nmap=node(mountain_mat,u.MaterialExpressionTextureSample,texture=normal,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
rmap=node(mountain_mat,u.MaterialExpressionTextureSample,texture=rough,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
for tex in [rock,nmap,rmap]: link(uv,tex,'Coordinates')
rockcolor=node(mountain_mat,u.MaterialExpressionMultiply)
link(vector(mountain_mat,'RockTint',(.48,.56,.65)),rockcolor,'B')
vn=node(mountain_mat,u.MaterialExpressionVertexNormalWS)
up=node(mountain_mat,u.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False)
link(vn,up,'Input')
clamp=node(mountain_mat,u.MaterialExpressionClamp,min_default=0,max_default=1); link(up,clamp,'Input')
slope=node(mountain_mat,u.MaterialExpressionPower,const_exponent=4); link(clamp,slope,'Base')
world=node(mountain_mat,u.MaterialExpressionWorldPosition)
# World-space triplanar albedo avoids stretching rock photographs up steep faces.
tri=[]; weights=[]
for channels,normal_channel in [((True,True,False),(False,False,True)),((True,False,True),(False,True,False)),((False,True,True),(True,False,False))]:
    coord=node(mountain_mat,u.MaterialExpressionComponentMask,r=channels[0],g=channels[1],b=channels[2],a=False); link(world,coord,'Input')
    tiled=node(mountain_mat,u.MaterialExpressionDivide); link(coord,tiled,'A'); link(scalar(mountain_mat,'RockTileCm',1600),tiled,'B')
    sample=node(mountain_mat,u.MaterialExpressionTextureSample,texture=diffuse); link(tiled,sample,'Coordinates')
    axis=node(mountain_mat,u.MaterialExpressionComponentMask,r=normal_channel[0],g=normal_channel[1],b=normal_channel[2],a=False); link(vn,axis,'Input')
    weight=node(mountain_mat,u.MaterialExpressionAbs); link(axis,weight,'Input'); weights.append(weight)
    weighted=node(mountain_mat,u.MaterialExpressionMultiply); link(sample,weighted,'A','RGB'); link(weight,weighted,'B'); tri.append(weighted)
def summed(nodes):
    first=node(mountain_mat,u.MaterialExpressionAdd); link(nodes[0],first,'A'); link(nodes[1],first,'B')
    second=node(mountain_mat,u.MaterialExpressionAdd); link(first,second,'A'); link(nodes[2],second,'B'); return second
tricolor=node(mountain_mat,u.MaterialExpressionDivide); link(summed(tri),tricolor,'A'); link(summed(weights),tricolor,'B')
link(tricolor,rockcolor,'A')
z=node(mountain_mat,u.MaterialExpressionComponentMask,r=False,g=False,b=True,a=False); link(world,z,'Input')
snowheight=node(mountain_mat,u.MaterialExpressionSubtract); link(z,snowheight,'A'); link(scalar(mountain_mat,'SnowStartCm',17000),snowheight,'B')
snowrange=node(mountain_mat,u.MaterialExpressionDivide); link(snowheight,snowrange,'A'); link(scalar(mountain_mat,'SnowTransitionCm',22000),snowrange,'B')
heightalpha=node(mountain_mat,u.MaterialExpressionClamp,min_default=0,max_default=1); link(snowrange,heightalpha,'Input')
alpha=node(mountain_mat,u.MaterialExpressionMultiply); link(slope,alpha,'A'); link(heightalpha,alpha,'B')
blend=node(mountain_mat,u.MaterialExpressionLinearInterpolate)
link(rockcolor,blend,'A'); link(vector(mountain_mat,'SnowColor',(.48,.56,.63)),blend,'B'); link(alpha,blend,'Alpha')
out(blend,mountain_mat,u.MaterialProperty.MP_BASE_COLOR)
out(rmap,mountain_mat,u.MaterialProperty.MP_ROUGHNESS)
normalblend=node(mountain_mat,u.MaterialExpressionLinearInterpolate)
link(vector(mountain_mat,'CliffNormal',(0,0,1)),normalblend,'A'); link(nmap,normalblend,'B','RGB'); link(slope,normalblend,'Alpha')
out(normalblend,mountain_mat,u.MaterialProperty.MP_NORMAL)
out(scalar(mountain_mat,'Specular',.22),mountain_mat,u.MaterialProperty.MP_SPECULAR)
finish(mountain_mat)

# Deterministic fractal height field, two ridgelines either side of an open valley.
def hash2(x,y):
    n=(x*374761393+y*668265263+19439)&0xffffffff
    n=((n^(n>>13))*1274126177)&0xffffffff
    return (n^(n>>16))/4294967295
def noise(x,y):
    ix=math.floor(x); iy=math.floor(y)
    fx=x-ix; fy=y-iy; fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy)
    a=hash2(ix,iy)*(1-fx)+hash2(ix+1,iy)*fx
    b=hash2(ix,iy+1)*(1-fx)+hash2(ix+1,iy+1)*fx
    return a*(1-fy)+b*fy
def fbm(x,y):
    return sum(noise(x*2**k,y*2**k)*(.5**k) for k in range(6))/1.96875
def height(x,y):
    valley=max(0,abs(y)-150)
    fade=(1-math.exp(-valley/650))**1.6
    warp=fbm(x/650+17,y/650-9)
    ridge=(1-abs(2*noise(x/520+warp*1.4,y/450)-1))**1.8
    detail=fbm(x/170,y/170)
    return max(0,fade*(140+600*ridge+240*detail)-5)
GRID=321; SIZE=6000
obj=art/'AlpineBackdrop_V2.obj'
with obj.open('w',encoding='utf-8') as f:
    f.write('o AlpineBackdrop_V2\n')
    for j in range(GRID):
        for i in range(GRID):
            x=(i/(GRID-1)-.5)*SIZE; y=(j/(GRID-1)-.5)*SIZE
            f.write('v %.4f %.4f %.4f\n'%(x*100,y*100,height(x,y)*100))
    for j in range(GRID):
        for i in range(GRID):
            x=(i/(GRID-1)-.5)*SIZE; y=(j/(GRID-1)-.5)*SIZE
            f.write('vt %.6f %.6f\n'%(x/12,y/12))
    for j in range(GRID):
        for i in range(GRID):
            x=(i/(GRID-1)-.5)*SIZE; y=(j/(GRID-1)-.5)*SIZE
            dx=(height(x+1,y)-height(x-1,y))/2; dy=(height(x,y+1)-height(x,y-1))/2
            inv=1/math.sqrt(dx*dx+dy*dy+1)
            f.write('vn %.6f %.6f %.6f\n'%(-dx*inv,-dy*inv,inv))
    for j in range(GRID-1):
        for i in range(GRID-1):
            a=j*GRID+i+1; b=a+1; c=a+GRID; d=c+1
            f.write('f %d/%d/%d %d/%d/%d %d/%d/%d\n'%(a,a,a,b,b,b,d,d,d))
            f.write('f %d/%d/%d %d/%d/%d %d/%d/%d\n'%(a,a,a,d,d,d,c,c,c))
meshpath=ROOT+'/Meshes/SM_AlpineBackdrop_V2'
task=u.AssetImportTask()
task.filename=str(obj); task.destination_path=ROOT+'/Meshes'; task.destination_name='SM_AlpineBackdrop_V2'
task.automated=True; task.replace_existing=True; task.save=True
tools.import_asset_tasks([task])
mountain=assets.load_asset(meshpath)
if not mountain: raise RuntimeError('Mountain import failed')
mountain.set_material(0,mountain_mat); assets.save_loaded_asset(mountain)
cube=assets.load_asset('/Engine/BasicShapes/Cube')
cylinder=assets.load_asset('/Engine/BasicShapes/Cylinder')
steel=instance('MI_LampSteel',(.035,.045,.055),.38)
base=instance('MI_LampBase',(.18,.19,.20),.85)
lamp=instance('MI_LampWarm',(.85,.72,.45),.3,10,(1,.62,.26))
def mesh_actor(name,mesh,location,scale=(1,1,1),mat=None,rotation=None,folder='Environment/AlpineV2'):
    a=by_label.get(name)
    if not a:
        a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*location),rotation or u.Rotator())
        by_label[name]=a
    a.set_actor_label(name); a.set_folder_path(folder)
    a.set_actor_location(u.Vector(*location),False,False)
    a.set_actor_scale3d(u.Vector(*scale))
    if rotation: a.set_actor_rotation(rotation,False)
    c=a.static_mesh_component; c.set_static_mesh(mesh)
    c.set_simulate_physics(False); c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    if mat: c.set_material(0,mat)
    return a
for label in ['AlpineBackdrop']:
    if label in by_label:
        old=by_label[label]; old.set_actor_hidden_in_game(True)
        old.static_mesh_component.set_visibility(False)
new=mesh_actor('AlpineBackdrop_Detailed',mountain,(0,0,-120),mat=mountain_mat)
bounds=mountain.get_bounding_box(); size=bounds.max-bounds.min
if size.y<200000 and size.z>400000: new.set_actor_rotation(u.Rotator(roll=90),False)
# Road lamps are editor assets, no runtime spawning or physics.
lamp_count=0
for side in [-1,1]:
    for i in range(34):
        x=-99000+i*6000; y=side*790
        key='StreetLamp_%d_%02d'%(side,i)
        mesh_actor(key+'_Base',cube,(x,y,10),(.6,.6,.2),base,folder='Lighting/StreetLamps')
        mesh_actor(key+'_Pole',cylinder,(x,y,450),(.14,.14,9),steel,folder='Lighting/StreetLamps')
        mesh_actor(key+'_Arm',cube,(x,side*655,892),(.16,2.8,.16),steel,folder='Lighting/StreetLamps')
        mesh_actor(key+'_Head',cube,(x,side*505,875),(.95,.60,.18),steel,folder='Lighting/StreetLamps')
        mesh_actor(key+'_Panel',cube,(x,side*505,864),(.80,.47,.025),lamp,folder='Lighting/StreetLamps')
        light=by_label.get(key+'_Light')
        if not light:
            light=actors.spawn_actor_from_class(u.SpotLight,u.Vector(x,side*480,855),u.Rotator(pitch=-90))
        light.set_actor_label(key+'_Light'); light.set_folder_path('Lighting/StreetLamps')
        lc=light.get_component_by_class(u.SpotLightComponent)
        lc.set_mobility(u.ComponentMobility.MOVABLE)
        lc.set_editor_property('intensity_units',u.LightUnits.LUMENS)
        lc.set_intensity(32000); lc.set_light_color(u.LinearColor(1,.73,.43))
        lc.set_attenuation_radius(2500); lc.set_inner_cone_angle(45); lc.set_outer_cone_angle(70)
        lc.set_cast_shadows(False)
        lc.set_editor_property('max_draw_distance',28000)
        lc.set_editor_property('max_distance_fade_range',6000)
        lamp_count+=1
for a in actors.get_all_level_actors():
    if isinstance(a,u.DirectionalLight):
        c=a.get_component_by_class(u.DirectionalLightComponent)
        c.set_intensity(400); c.set_light_color(u.LinearColor(1,.83,.65))
        a.set_actor_rotation(u.Rotator(pitch=-8,yaw=-45),False)
    if isinstance(a,u.PostProcessVolume):
        s=a.settings; s.auto_exposure_min_brightness=8; s.auto_exposure_max_brightness=8
        s.auto_exposure_bias=-.15; s.bloom_intensity=.10; a.settings=s
if not levels.save_current_level(): raise RuntimeError('Level save failed')
assets.save_directory(ROOT,only_if_is_dirty=True,recursive=True)
report={'map':MAP,'street_lights':lamp_count,'mountain_grid':GRID,'mountain_triangles':2*(GRID-1)**2,
        'mesh_bounds_cm':str(size),'old_mountain_preserved':True,
        'rock_source':'https://polyhaven.com/a/rock_boulder_cracked','license':'CC0',
        'normal':'DirectX','texture_resolution':2048,'unreal_units':'cm'}
(project/'Saved'/'alpine-v2-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('ALPINE_V2_COMPLETE '+json.dumps(report))

