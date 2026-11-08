"""Run inside Unreal's Python console. Duplicate original map before any art edits.
Geometry CSV is exported by the same C++ header used by runtime/collision.
"""
import unreal as u
from pathlib import Path
import csv,math,json,itertools
project=Path(u.Paths.project_dir())
art=project/'SourceArt'/'Ramp'
rows=[{k:float(v) for k,v in r.items()} for r in csv.DictReader((art/'road.csv').open())]
assets=u.EditorAssetLibrary; tools=u.AssetToolsHelpers.get_asset_tools()
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
SOURCE='/Game/Maps/IceTrackDrive'; MAP='/Game/Maps/IceRampDrive'
refresh=globals().get('RAMP_REFRESH',False)
if assets.does_asset_exist(MAP) and not refresh: raise RuntimeError('Refusing to overwrite existing ramp map')
if not levels.load_level(MAP if refresh else SOURCE): raise RuntimeError('Cannot load map')
if refresh:
    for generated in list(actors.get_all_level_actors()):
        if str(generated.get_folder_path())=='VirtualRamp': actors.destroy_actor(generated)
before=list(actors.get_all_level_actors()); hidden=[]
prefixes=('TestSurface','RoadLane','RoadEdge','RoadShoulder','RoadWall','ThreeLane','WallReflector','StreetLamp','Runway','CenterDash','BrakeDistance','Distance_','Skidpad')
for a in before:
    label=a.get_actor_label()
    if label.startswith(prefixes) or str(a.get_folder_path()).startswith('Track'):
        a.set_actor_hidden_in_game(True); a.set_is_temporarily_hidden_in_editor(True)
        for c in a.get_components_by_class(u.PrimitiveComponent):
            c.set_visibility(False); c.set_hidden_in_game(True)
        hidden.append(label)
    if label=='AlpineBackdrop_Detailed' and not refresh:
        loc=a.get_actor_location(); loc.y+=14000
        a.set_actor_location(loc,False,False)
    if isinstance(a,u.PlayerStart): a.set_actor_location(u.Vector(500,0,2020),False,False)

def point(row,lift,lateral):
    h=row['heading']; x=row['x']-math.sin(h)*lateral; y=row['y']+math.cos(h)*lateral
    return (x*100,-y*100,(row['z']+row['bank']*lateral+lift)*100)

class Mesh:
    def __init__(self,name): self.name=name;self.v=[];self.uv=[];self.f=[]
    def quad(self,p,uv,double=False):
        n=len(self.v)+1;self.v.extend(p);self.uv.extend(uv)
        self.f.extend([(n,n+1,n+2),(n,n+2,n+3)])
        # Back faces need their own vertices, otherwise normal recomputation
        # averages opposite normals to zero and causes black striped shading.
        if double: self.quad(list(reversed(p)),list(reversed(uv)),False)
    def strip(self,a,b,l0,l1,lift=0):
        l0,l1=sorted((l0,l1))
        self.quad([point(a,lift,l0),point(a,lift,l1),point(b,lift,l1),point(b,lift,l0)],[(a['s']/5,l0/5),(a['s']/5,l1/5),(b['s']/5,l1/5),(b['s']/5,l0/5)])
    def save(self):
        p=art/(self.name+'.obj')
        with p.open('w') as f:
            f.write('o '+self.name+'\n')
            for v in self.v: f.write('v %.6f %.6f %.6f\n'%tuple(v))
            for uv in self.uv: f.write('vt %.6f %.6f\n'%uv)
            f.write('s 1\n')
            for t in self.f: f.write('f '+' '.join('%d/%d'%(i,i) for i in t)+'\n')
        return p

road=Mesh('SM_VirtualRamp_Road'); paint=Mesh('SM_VirtualRamp_Paint')
walls=Mesh('SM_VirtualRamp_Walls'); shoulder=Mesh('SM_VirtualRamp_Shoulder');fill=Mesh('SM_VirtualRamp_Embankment')
for a,b in zip(rows,rows[1:]):
    road.strip(a,b,-6.75,6.75)
    for side in (-1,1):
        shoulder.strip(a,b,side*5.25,side*6.75,.005)
        paint.strip(a,b,side*5.25-.075,side*5.25+.075,.018)
        if int(a['s'])%9<3: paint.strip(a,b,side*1.75-.075,side*1.75+.075,.019)
        inner=side*6.75;outer=side*7.15
        walls.quad([point(a,0,inner),point(b,0,inner),point(b,1.2,inner),point(a,1.2,inner)],[(0,0),(1,0),(1,1),(0,1)],True)
        walls.strip(a,b,min(inner,outer),max(inner,outer),1.2)
        walls.quad([point(a,0,outer),point(a,1.2,outer),point(b,1.2,outer),point(b,0,outer)],[(0,0),(0,1),(1,1),(1,0)],True)
        p0=point(a,0,outer);p1=point(b,0,outer)
        q0=list(point(a,0,side*45));q1=list(point(b,0,side*45));q0[2]=q1[2]=-50
        fill.quad([p0,p1,q1,q0],[(a['s']/10,0),(b['s']/10,0),(b['s']/10,4),(a['s']/10,4)],True)

def spawn(name,mesh,loc=(0,0,0),scale=(1,1,1),mat=None,rot=None):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*loc),rot or u.Rotator())
    a.set_actor_label(name); a.set_folder_path('VirtualRamp')
    a.set_actor_scale3d(u.Vector(*scale));c=a.static_mesh_component
    c.set_static_mesh(mesh);c.set_simulate_physics(False);c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    if mat: c.set_material(0,mat)
    return a

mesh_checks=[]
for m,mat in [(road,'MI_AsphaltRoad'),(paint,'MI_TrackPaint'),(walls,'MI_RoadConcrete'),(shoulder,'MI_AsphaltShoulder'),(fill,'MI_Snow')]:
    task=u.AssetImportTask();task.filename=str(m.save());task.destination_path='/Game/Driving/Ramp'
    task.destination_name=m.name;task.automated=True;task.save=True;task.replace_existing=True
    tools.import_asset_tasks([task]);asset=assets.load_asset('/Game/Driving/Ramp/'+m.name)
    if not asset: raise RuntimeError('Mesh import failed '+m.name)
    material=assets.load_asset('/Game/Driving/Materials/'+mat)
    asset.set_material(0,material);assets.save_loaded_asset(asset)
    actor=spawn(m.name,asset)
    # OBJ import has historically changed axes in this engine build. Fit the
    # import's rigid axis conversion against the source bounds; never guess roll.
    box=asset.get_bounding_box()
    corners=list(itertools.product((box.min.x,box.max.x),(box.min.y,box.max.y),(box.min.z,box.max.z)))
    expected=[min(v[k] for v in m.v) for k in range(3)]+[max(v[k] for v in m.v) for k in range(3)]
    best=None
    for angle in (0,90,-90,180):
        rot=u.Rotator(roll=angle);q=rot.quaternion()
        for sy,sz in itertools.product((1,-1),repeat=2):
            transformed=[q.rotate_vector(u.Vector(x,y*sy,z*sz)) for x,y,z in corners]
            got=[min(getattr(v,k) for v in transformed) for k in ('x','y','z')]+[max(getattr(v,k) for v in transformed) for k in ('x','y','z')]
            error=max(abs(x-y) for x,y in zip(got,expected))
            if best is None or error<best[0]: best=(error,angle,sy,sz)
    if best[0]>.1: raise RuntimeError('Imported mesh does not match CSV bounds '+str(best))
    actor.set_actor_rotation(u.Rotator(roll=best[1]),False)
    actor.set_actor_scale3d(u.Vector(1,best[2],best[3]))
    mesh_checks.append({'mesh':m.name,'bound_error_cm':best[0],'roll':best[1],'scale':[1,best[2],best[3]],'triangles':len(m.f)})

cube=assets.load_asset('/Engine/BasicShapes/Cube');cylinder=assets.load_asset('/Engine/BasicShapes/Cylinder')
steel=assets.load_asset('/Game/Driving/Materials/MI_LampSteel')
lamp=assets.load_asset('/Game/Driving/Materials/MI_LampWarm')
amber=assets.load_asset('/Game/Driving/Materials/MI_LightAmber')
for i in range(15,len(rows)-1,30):
    r=rows[i]; h=r['heading'];c=math.cos(h);sn=math.sin(h)
    for side in (-1,1):
        x,y,z=point(r,0,side*7.6)
        spawn('RampLamp_%d_%d_Pole'%(side,i),cylinder,(x,y,z+450),(.14,.14,9),steel)
        arm=point(r,8.92,side*6.5)
        spawn('RampLamp_%d_%d_Arm'%(side,i),cube,arm,(.16,2.8,.16),steel,u.Rotator(yaw=-h*180/math.pi))
        head=point(r,8.75,side*5.05)
        spawn('RampLamp_%d_%d_Head'%(side,i),cube,head,(.95,.6,.18),steel,u.Rotator(yaw=-h*180/math.pi))
        panel=point(r,8.64,side*5.05)
        spawn('RampLamp_%d_%d_Panel'%(side,i),cube,panel,(.8,.47,.025),lamp,u.Rotator(yaw=-h*180/math.pi))
        marker=point(r,.75,side*6.735)
        spawn('RampReflector_%d_%d'%(side,i),cube,marker,(.12,.025,.25),amber,u.Rotator(yaw=-h*180/math.pi))

# Same lamp meshes/materials; emissive fixtures avoid dozens of new dynamic lights on Android.
if not u.EditorLoadingAndSavingUtils.save_map(u.EditorLevelLibrary.get_editor_world(),MAP): raise RuntimeError('Save ramp copy failed')
assets.save_directory('/Game/Driving/Ramp')
u.EditorLevelLibrary.set_level_viewport_camera_info(u.Vector(19000,33000,23000),u.Rotator(pitch=-32,yaw=-12,roll=0))
report={'map':MAP,'source':SOURCE,'fictional':True,'nodes':len(rows),'length_m':rows[-1]['s'],'radius_m':150,'transition_m':60,'bank':.04,'grade':-.03,'lanes':3,'lane_width':3.5,'paved_width':13.5,'hidden_original_actors':hidden,'mesh_checks':mesh_checks}
(project.parent.parent/'docs'/'research'/'virtual-ramp-art.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('VIRTUAL_RAMP_CREATED '+json.dumps({k:v for k,v in report.items() if k!='hidden_original_actors'}))
