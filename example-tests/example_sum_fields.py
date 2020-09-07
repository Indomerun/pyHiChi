import sys
sys.path.append("../bin")
import pyHiChi as hichi
import numpy as np


min_coords = hichi.Vector3d(-10, -10, 0.0)
max_coords = hichi.Vector3d(10, 10, 0.0)

    
def null_value(x, y, z):
    return 0.0


# ------- create the first pulse ---------------

def field_value_1(x, y, z):
    return np.exp(-x**2-y**2)*np.sin(3*x)  # omega=3
    
grid_size_1 = hichi.Vector3d(128, 128, 1)
d1 = (max_coords.x - min_coords.x) / grid_size_1.x
grid_step_1 = hichi.Vector3d(d1, d1, d1)
time_step_1 = 0.05/hichi.c

field_1 = hichi.PSATDField(grid_size_1, min_coords, grid_step_1, time_step_1)
field_1.set_E(null_value, field_value_1, null_value)
field_1.set_B(null_value, null_value, field_value_1)
field_1.convert_fields_poisson_equation()


# ------- create the second pulse --------------

def field_value_2(x, y, z):
    return np.exp(-x**2-y**2)*np.sin(6*x)  # omega=6
    
grid_size_2 = hichi.Vector3d(256, 256, 1)
d2 = (max_coords.x - min_coords.x) / grid_size_2.x
grid_step_2 = hichi.Vector3d(d2, d2, d2)
time_step_2 = 0.025/hichi.c  # PSTD Courant condition is taken into account

field_2 = hichi.PSTDField(grid_size_2, min_coords, grid_step_2, time_step_2)
field_2.set_E(null_value, field_value_2, null_value)
field_2.set_B(null_value, null_value, field_value_2)


# ------ transform fields ---------------------

rotation_mapping_z_axis = hichi.RotationMapping(hichi.Axis.Z, hichi.pi/3)
shift_mapping_1 = hichi.ShiftMapping(hichi.Vector3d(-6.5, -4.0, 0.0))
shift_mapping_2 = hichi.ShiftMapping(hichi.Vector3d(5.0, -4.0, 0.0))
shift_mapping_3 = hichi.ShiftMapping(hichi.Vector3d(-7.0, 7.0, 0.0))

res_field = (0.5*field_2.apply_mapping(shift_mapping_1) + field_1 + (field_2*0.8).apply_mapping(shift_mapping_2))\
                .apply_mapping(rotation_mapping_z_axis) + field_1.apply_mapping(shift_mapping_3)


# ------ update fields ------------------------

def update_data():  # dt = 0.05/hichi.c
    field_1.update_fields()  # dt = 0.05/hichi.c
    field_2.update_fields()  # dt = 0.025/hichi.c
    field_2.update_fields()  # dt = 0.025/hichi.c
    
    
# --------- show -------------

import matplotlib.pyplot as plt
import matplotlib.animation as animation

N = 128
x = np.linspace(min_coords.x, max_coords.x, N)
y = np.linspace(min_coords.y, max_coords.y, N)

def get_fields(field):
    global x, y, N
    res = np.zeros(shape=(N,N))
    for ix in range(N):
        for iy in range(N):
            coord_xy = hichi.Vector3d(x[ix], y[iy], 0.0)
            res[N - iy - 1, ix] = field.get_E(coord_xy).norm()
    return res

fig = plt.figure()
ax = fig.add_subplot(1,1,1)

im = ax.imshow(get_fields(res_field), cmap='RdBu', interpolation='none',
    extent=(min_coords.x, max_coords.x, min_coords.y, max_coords.y), animated = True)
fig.colorbar(im, ax=ax)
ax.set_xlabel("x")
ax.set_ylabel("y")

def update_fig(*args):
    update_data()
    im.set_array(get_fields(res_field))
    return im,
    
ani = animation.FuncAnimation(fig, update_fig, interval=10, blit=True)

fig.tight_layout()

plt.show()


