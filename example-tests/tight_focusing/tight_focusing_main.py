import sys
sys.path.append("../../python_modules")
sys.path.append("../../bin")
import pyHiChi as hichi
from tight_focusing_fields import SphericalPulseC, SphericalPulsePython
import math as ma


# ------------------- initializing -------------------------------------


factor = 0.5
NxFull = int(factor*320)                           # size of grid in full area
Ny = int(factor*256)
Nz = int(factor*256)
NxBand = int(56*factor)                            # size of grid in the band
gridSize = hichi.vector3d(NxBand, Ny, Nz)          # real size of grid

# creating of spherical pulse
sphericalPulse = SphericalPulsePython(f_number = 0.3,
                                      R0 = 16,
                                      pulselength = 2.0,
                                      phase = 0,
                                      edgeSmoothingAngle = 0.1)

timeStep = 1.0/hichi.c                             # time step in CGS system of units
maxIter = 32                                       # number of iterations to compute       

minCoords = hichi.vector3d(-20, -20, -20)          # bounds of full area
maxCoords = hichi.vector3d(20, 20, 20)

D = 3.5*sphericalPulse.pulselength                 # band width

# to compute the task in the full area just set
# D = maxCoords.x - minCoords.x
                                                   
                                                   
# creating of mapping
mapping = hichi.TightFocusingMapping(sphericalPulse.R0, sphericalPulse.pulselength, D)

# creating of mapping with cutting at angle
# cutAngle = sphericalPulse.openingAngle + sphericalPulse.edgeSmoothingAngle
# mapping = hichi.TightFocusingMapping(sphericalPulse.R0, sphericalPulse.pulseLength, D, cutAngle)

# not to cut secondary pulses
# mapping.setIfCut(False)

xMin = mapping.getxMin()  # bounds of the band
xMax = mapping.getxMax()

# computing of step of grid
gridMinCoords = hichi.vector3d(xMin, minCoords.y, minCoords.z)
gridMaxCoords = hichi.vector3d(xMax, maxCoords.y, maxCoords.z)
gridStep = (gridMaxCoords - gridMinCoords) / gridSize

# creating of grid for PSATDGridMapping
grid = hichi.PSATDGridMapping(gridSize, timeStep, gridMinCoords, gridStep)
grid.setMapping(mapping)

# creating of field solver PSATD for existing grid
# the Poisson equation will be satisfied
fieldSolver = hichi.PSATDPoisson(grid)


def initialize():
    # setting of start conditions
    sphericalPulse.setField(grid)


def updateFields():
    # doing one iteration of PSATD
    fieldSolver.updateFields()


# ----------- run and show animation (size of grid should be not large) ------

from hichi_visualisation import *
from hichi_primitives import Axis, Plane, Field

visual = Visual(grid, minCoords, maxCoords, dpi=500, fontsize=17)

def animateInPlane(visual, nIter):
    visual.animateInPlane(shape=(NxFull*2, Ny*2), funcUpdate=updateFields, nIter=nIter,
                          plane=Plane.XOY, lastCoordinateValue=0.0,
                          field=Field.E, norm=True,
                          valueLimits=(0.0, 0.5),
                          interval=1
                          )
                            
def animateInAxis(visual, nIter):
    visual.animateInAxis(nPoints=NxFull*2, funcUpdate=updateFields, nIter=nIter,
                         axis=Axis.X, lastCoordinateValue=(0.0, 0.0),
                         field=Field.E, norm=True,
                         yLimits=(-1.0, 8.0),
                         interval=50
                         )                            
                            
initialize()
animateInPlane(visual, maxIter)  # it should be the last function in the current script
#animateInAxis(visual, maxIter)  # it should be the last function in the current script


# ----------- run and save pictures for every iteration ----------------------
'''
from hichi_visualisation import *
from hichi_primitives import Axis, Plane, Field, createDir

createDir("./pictures")
visual = Visual(grid, minCoords, maxCoords, "./pictures", dpi=500, fontsize=17)

def savePicInPlane(visual, iter):
    visual.savePictureInPlane(shape=(NxFull*4, Ny*4), plane=Plane.XOY, lastCoordinateValue=0.0,
                              field=Field.E, norm=True,
                              valueLimits=(0.0, 0.5),
                              namePicture="field%04d.png" % iter
                              )
                            
def savePicInAxis(visual, iter):
    visual.savePictureInAxis(nPoints=NxFull*4, axis=Axis.X, lastCoordinateValue=(0.0, 0.0),
                             field=Field.E, norm=True,
                             yLimits=(-1.0, 8.0),
                             namePicture="field%04d.png" % iter
                             )                            
                            
initialize()
for i in range(maxIter):
    savePicInPlane(visual, i)
    #savePicInAxis(visual, i)
    updateFields()
'''

# ----------- run and save results in .csv files --------------------------
'''
from hichi_writing import Writer, Reader
from hichi_primitives import Axis, Plane, Field, createDir

createDir("./csv")
writer = Writer(grid, minCoords, maxCoords, "./csv")

def saveFileInPlane(writer, iter):
    writer.saveFileInPlane(shape=(NxFull*4, Ny*4), plane=Plane.XOY, lastCoordinateValue=0.0,
                           field=Field.E, norm=True,
                           nameFile="field%04d.csv" % iter
                           )
                            
def saveFileInAxis(writer, iter):
    writer.saveFileInAxis(nPoints=NxFull*4, axis=Axis.X, lastCoordinateValue=(0.0, 0.0),
                          field=Field.E, norm=True,
                          nameFile="field%04d.csv" % iter
                          )                            
                            
initialize()
for i in range(maxIter):
    saveFileInPlane(writer, i)
    #saveFileInAxis(writer, i)
    updateFields()

# it is possible to read crated file to numpy.array   
import numpy as np
reader = Reader("./csv")
field = reader.readFile2d("field0000.csv")
#field = reader.readFile1d("field0000.csv")
print(field)
'''
