import unreal, json
from pathlib import Path
result={}
for name in ('SM_SportsCar','SM_SportsCar_Glass','SM_SportsCar_Wheel','SKM_SportsCar'):
    mesh=unreal.load_asset('/Game/Vehicles/SportsCar/'+name)
    if not mesh:
        result[name]={'error':'not loaded'}
        continue
    item={'class':mesh.get_class().get_name()}
    if isinstance(mesh,unreal.StaticMesh):
        bounds=mesh.get_bounding_box()
        item['min']=str(bounds.min); item['max']=str(bounds.max)
        item['materials']=[str(m.material_interface.get_path_name()) if m.material_interface else None for m in mesh.static_materials]
    result[name]=item
Path(unreal.Paths.project_saved_dir(),'vehicle-assets.json').write_text(json.dumps(result,indent=2))
unreal.log(str(result))
