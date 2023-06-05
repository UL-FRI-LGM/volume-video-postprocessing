import numpy as np
import pyopenvdb as vdb

def pad_to(a, shape):
    x = shape[0] - a.shape[0]
    y = shape[1] - a.shape[1]
    z = shape[2] - a.shape[2]
    return np.pad(a, ((x//2, x - x//2), (y//2, y - y//2), (z//2, z - z//2)), 'constant')

volume = vdb.read('/home/enei/Documents/diplomska/model_vdb/monkey.vdb', 'density')

vdb_shape = volume.evalActiveVoxelBoundingBox()
shape = list(np.array(vdb_shape[1]) - np.array(vdb_shape[0]))
out = np.ndarray(shape, int)
#acc = volume.getConstAccessor()

#for i in range(shape[0]):
#    for j in range(shape[1]):
#        for k in range(shape[2]):
#            print(list( map(int, np.array([i,j,k], int) - np.array(vdb_shape[0], int)) ))
#            if acc.probeValue(list( map(int, np.array([i,j,k], int) - np.array(vdb_shape[0], int)) ))[1]:
#                out[i,j,k] = 1
#            
#print(np.unique(out))
volume.copyToArray(out, ijk=vdb_shape[0])
print(volume.activeVoxelCount())
volume.clear()
volume.copyFromArray(out)
print(volume.activeVoxelCount())

print(out.shape)

print(volume.activeVoxelCount())
print(out.shape)
out = pad_to(out, (128, 128, 128))
out[np.where(out == 1)] = 255
print(np.unique(out, return_counts=True))
print(out.shape)
out.astype(np.uint8).tofile('/home/enei/Documents/diplomska/model_raw/monkey.bin')
#out.tofile('/home/enei/Documents/diplomska/monkey.bin')
