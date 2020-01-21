import sys
import os
sys.path.append("../../build/src/pyHiChi/Release")
import pyHiChi as hichi
import numpy as np

def getFields(grid):
    field = np.zeros(shape=(Ny,Nx))
    for iy in range(Ny):
        for ix in range(Nx):
            coordXY = hichi.vector3d(x[ix], y[iy], 0.0)
            E = grid.getE(coordXY)
            field[iy, ix] = E.norm()
    return field

def write(grid, update, minCoords, maxCoords, Nx = 300, Ny = 300, maxIter=160, dumpIter = 20, fileName = "res_x_%d.csv", dirResult = "./results/"):
    
    x = np.arange(minCoords.x, maxCoords.x, (maxCoords.x - minCoords.x)/Nx)
    y = np.arange(minCoords.y, maxCoords.y, (maxCoords.y - minCoords.y)/Ny)
    
    for iter in range(maxIter + 1):
        print("\r %d" % iter),
        update()
        field = getFields(grid)
        if (iter % dumpIter == 0):
            with open(dirResult + fileName % iter, "w") as file:
                for j in range(Ny):
                    for i in range(Nx):
                        file.write(str(field[j, i])+";")
                    file.write("\n")  
    print


def writeOX(grid, update, minCoords, maxCoords, Nx = 300, maxIter=160, dumpIter = 20, fileName = "res_x_%d.csv",\
    dirResult = "./results/", ifWriteZeroIter = True):
    
    x = np.arange(minCoords.x, maxCoords.x, (maxCoords.x - minCoords.x)/Nx)

    for iter in range(maxIter + 1):
        update()
        if ((iter == 0 and ifWriteZeroIter) or (iter != 0 and iter % dumpIter == 0)):
            field = getFields(grid)
            with open(dirResult + fileName % iter, "w") as file:
                for i in range(Nx):
                    file.write(str(field[i])+"\n")  
    print
