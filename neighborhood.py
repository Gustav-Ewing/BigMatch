from utils import *

def loadNeighborhood():
	global R
	R = loaddump(join(BIN_FOLDER, "rangedic.bin"))

#### loads the dictionary of neighbors based on range and then returns the neighborhood for a specific consumer/prosumer with the specified range
def findNeighborhood(dist, index):
	return R[dist][index]

