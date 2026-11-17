import bpy,math,json,pathlib
from mathutils import Vector,Matrix
from bpy_extras.io_utils import axis_conversion
root=pathlib.Path(__file__).resolve().parents[2];out=root/'work/pincer3d';assets=root/'CarryOn/assets/pincer3d'
bpy.ops.wm.open_mainfile(filepath=str(out/'source.blend'))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH');bpy.context.view_layer.objects.active=o;o.select_set(True)
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
turn=Matrix.Rotation(math.radians(-30),3,'Y')
for v in o.data.vertices:v.co=turn@v.co
lo=min(v.co.z for v in o.data.vertices);hi=max(v.co.z for v in o.data.vertices);scale=.62/(hi-lo);grip=lo+(hi-lo)*.28
# Center the handle at the palm; the lower 28 percent remains below the grip.
handle=[v.co.x for v in o.data.vertices if abs(v.co.z-grip)<(hi-lo)*.025];cx=sum(handle)/len(handle)
for v in o.data.vertices:v.co=Vector(((v.co.x-cx)*scale,v.co.y*scale,(v.co.z-grip)*scale))
mod=o.modifiers.new('Handheld mesh budget','DECIMATE');mod.ratio=5000/len(o.data.polygons);bpy.ops.object.modifier_apply(modifier=mod.name)
tri=o.modifiers.new('Triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
for p in o.data.polygons:p.use_smooth=True
mat=bpy.data.materials.new('CarryOn brass and wrapped grip');mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF')
for slot,file,space in [('Base Color','albedo.png','sRGB'),('Metallic',next((assets/'source').glob('*metallic.png')), 'Non-Color'),('Roughness',next((assets/'source').glob('*roughness.png')), 'Non-Color')]:
 p=out/file if isinstance(file,str) else file
 tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(p));tex.image.colorspace_settings.name=space;mat.node_tree.links.new(tex.outputs['Color'],bs.inputs[slot])
tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(out/'normal.png'));tex.image.colorspace_settings.name='Non-Color';nm=mat.node_tree.nodes.new('ShaderNodeNormalMap');mat.node_tree.links.new(tex.outputs['Color'],nm.inputs['Color']);mat.node_tree.links.new(nm.outputs['Normal'],bs.inputs['Normal'])
o.data.materials.clear();o.data.materials.append(mat)
source_to_blender=axis_conversion(from_forward='-Z',from_up='Y').to_4x4()@Matrix.Diagonal((-1.,1.,1.,1.));to_game=source_to_blender.inverted().to_3x3()
records=[];indices=[];lods=[]
for target in [5000,1500,500]:
 obj=o.copy();obj.data=o.data.copy();bpy.context.collection.objects.link(obj);bpy.context.view_layer.objects.active=obj
 if target<5000:
  dec=obj.modifiers.new('LOD','DECIMATE');dec.ratio=target/len(obj.data.polygons);bpy.ops.object.modifier_apply(modifier=dec.name)
 mesh=obj.data;mesh.calc_loop_triangles();mesh.calc_tangents();uv=mesh.uv_layers.active.data;start=len(indices)
 for tri in mesh.loop_triangles:
  for li in tri.loops:
   l=mesh.loops[li];v=to_game@mesh.vertices[l.vertex_index].co;n=(to_game@l.normal).normalized();t=(to_game@l.tangent).normalized()
   indices.append(len(records));records.append([list(v),list(n),list(t),-l.bitangent_sign,[uv[li].uv.x,1-uv[li].uv.y]])
 lods.append([start,len(indices)-start]);bpy.data.objects.remove(obj,do_unlink=True)
(assets/'mesh.json').write_text(json.dumps({'records':records,'indices':indices,'lods':lods},separators=(',',':')))
bpy.context.view_layer.objects.active=o
for im in bpy.data.images:
 if im.source=='FILE':im.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(assets/'pincer.blend'))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
bpy.ops.object.camera_add(location=(.7,-1.8,.6));cam=bpy.context.object;scene.camera=cam;center=Vector((0,0,.13));cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=.85
for loc,power,size in [((1,-2,2),180,2),((-1,-1,1),100,1),((0,1,2),200,1)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(center-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.12,.12,.12,1)
scene.render.resolution_x=700;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.render.filepath=str(assets/'preview.png');bpy.ops.render.render(write_still=True)
print('EXPORT',len(records),'vertices',len(indices)//3,'triangles; LODs',lods)
