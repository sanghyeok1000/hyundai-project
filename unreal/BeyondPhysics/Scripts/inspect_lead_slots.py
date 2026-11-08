import unreal as u,json
from pathlib import Path
mesh=u.EditorAssetLibrary.load_asset('/Game/Vehicles/SportsCar/SM_SportsCar')
slots=[{'index':i,'slot':str(s.material_slot_name),'material':s.material_interface.get_path_name() if s.material_interface else ''} for i,s in enumerate(mesh.get_editor_property('static_materials'))]
(Path(u.Paths.project_dir()).resolve().parents[1] / 'docs/research/fog-lead-slots.json').write_text(json.dumps(slots,indent=2))
