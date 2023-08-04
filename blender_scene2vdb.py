import bpy
import numpy as np
import os

PATH = '/home/enei/Documents/diplomska/vdb/'
FRAME_COUNT = 60
if not os.path.exists(PATH):
    os.makedirs(PATH)

def mesh_to_volume(object, intensity, name):
    volume_data = bpy.data.volumes.new("output_volume")
    volume = bpy.data.objects.new("output_volume_object", volume_data)
    scene = bpy.context.scene
    scene.collection.objects.link(volume)
    
    transform = volume.modifiers.new('transform', 'MESH_TO_VOLUME')
    transform.object = object
    transform.density = intensity
    transform.resolution_mode = 'VOXEL_SIZE'
    transform.use_fill_volume = True
    transform.voxel_size = 0.05
    
    out = volume.evaluated_get(bpy.context.evaluated_depsgraph_get())
    print(out.data.grids.keys())
    
    out.data.grids.save(name)
    
    bpy.data.objects.remove(volume, do_unlink=True)


objects = bpy.context.scene.objects
objects = [obj for obj in objects if obj.type == 'MESH']
for i in range(FRAME_COUNT):
    bpy.context.scene.frame_set(i)
    frame_dir = PATH + f'{i}'
    if not os.path.exists(frame_dir):
        os.makedirs(frame_dir)
    for object in objects:
        color = np.array(object.data.materials[0]
                    .node_tree.nodes["Principled BSDF"]
                    .inputs["Base Color"].default_value)
        [r,g,b,_] = color         
        mesh_to_volume(object, 1.0, frame_dir + f'/{object.name}-{r},{g},{b}.vdb')
