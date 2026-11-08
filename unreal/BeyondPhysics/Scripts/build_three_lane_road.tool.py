import json
S="editor_toolset.toolsets.scene.SceneTools."
A="editor_toolset.toolsets.actor.ActorTools."
O="editor_toolset.toolsets.object.ObjectTools."
M="editor_toolset.toolsets.material_instance.MaterialInstanceTools."
F="editor_toolset.toolsets.asset.AssetTools."
def call(name,args):
    result=execute_tool(name,json.dumps(args))
    value=result.get("returnValue")
    if value is False: raise RuntimeError(name+" returned false")
    return value
def ref(path): return {"refPath":path}
def props(instance,values): return call(O+"set_properties",{"instance":instance,"values":json.dumps(values)})
def components(actor): return call(A+"get_components",{"actor":actor,"component_type":ref("/Script/Engine.StaticMeshComponent")})
def find(name): return call(S+"find_actors",{"name":name,"tag":"","collision_channels":[]})
def material(name,color,roughness):
    path="/Game/Driving/Materials/"+name
    # exists may return false; use direct execute for this query.
    exists=execute_tool(F+"exists",json.dumps({"path":path})).get("returnValue")
    if not exists: call(F+"duplicate",{"path":"/Game/Driving/Materials/MI_Charcoal","new_path":path})
    obj=ref(path+"."+name)
    call(M+"set_vector_parameter",{"instance":obj,"name":"Color","value":{"r":color[0],"g":color[1],"b":color[2],"a":1}})
    call(M+"set_scalar_parameter",{"instance":obj,"name":"Roughness","value":roughness})
    call(M+"set_scalar_parameter",{"instance":obj,"name":"Glow","value":0})
    return obj
def cube(name,x,y,z,sx,sy,sz,mat,shadow=False):
    matches=find(name)
    actor=matches[0] if matches else call(S+"add_to_scene_from_asset",{"asset_path":"/Engine/BasicShapes/Cube.Cube","name":name,"xform":{"location":{"x":x,"y":y,"z":z},"scale":{"x":sx,"y":sy,"z":sz}},"snap_to_ground":False})
    call(A+"set_actor_transform",{"actor":actor,"xform":{"location":{"x":x,"y":y,"z":z},"scale":{"x":sx,"y":sy,"z":sz}},"worldspace":True})
    for comp in components(actor): props(comp,{"overrideMaterials":[mat],"castShadow":shadow,"bVisible":True,"bHiddenInGame":False})
    return actor
def run():
    if call(S+"get_current_level",{})!="/Game/Maps/IceTrackDrive": raise RuntimeError("Wrong level")
    call(F+"save_assets",{"asset_paths":[]})
    asphalt=material("MI_AsphaltRoad",(.025,.029,.033),.92)
    concrete=material("MI_RoadConcrete",(.22,.235,.25),.92)
    shoulder=material("MI_AsphaltShoulder",(.039,.043,.048),.95)
    paint=ref("/Game/Driving/Materials/MI_TrackPaint.MI_TrackPaint")
    amber=ref("/Game/Driving/Materials/MI_LightAmber.MI_LightAmber")
    road=ref("/Game/Maps/IceTrackDrive.IceTrackDrive:PersistentLevel.StaticMeshActor_0")
    call(A+"set_actor_transform",{"actor":road,"xform":{"location":{"x":0,"y":0,"z":-8},"scale":{"x":2000,"y":13.5,"z":.16}},"worldspace":True})
    for comp in components(road): props(comp,{"overrideMaterials":[asphalt]})
    cube("RoadSurroundings_Frost",0,0,-18,2000,600,.16,ref("/Game/Driving/Materials/MI_Snow.MI_Snow"))
    hidden=0
    for prefix in ["RoadLaneDash_","RoadEdgeWhite_","RoadShoulder_"]:
        for actor in find(prefix):
            props(actor,{"bHidden":True})
            for comp in components(actor): props(comp,{"bVisible":False})
            hidden+=1
    for side in [-1,1]:
        cube("ThreeLaneShoulder_"+str(side),0,side*600,.4,2000,1.5,.008,shoulder)
        cube("ThreeLaneEdge_"+str(side),0,side*525,1.2,2000,.15,.012,paint)
        cube("RoadWall_"+str(side),0,side*695,60,2000,.4,1.2,concrete,True)
        cube("RoadWallCap_"+str(side),0,side*695,121,2000,.45,.04,paint)
        for i in range(221):
            cube("ThreeLaneDash_"+str(side)+"_"+str(i).zfill(3),-99000+i*900,side*175,1.3,3,.15,.012,paint)
        for i in range(81):
            cube("WallReflector_"+str(side)+"_"+str(i).zfill(3),-98000+i*2450,side*674.5,75,.12,.025,.25,amber)
    call(F+"save_assets",{"asset_paths":[]})
    return {"map":"/Game/Maps/IceTrackDrive","road_length_m":2000,"lanes":3,"lane_width_m":3.5,"wall_inner_y_m":[-6.75,6.75],"old_markers_hidden":hidden,"material":"MI_AsphaltRoad (no ice textures)"}

