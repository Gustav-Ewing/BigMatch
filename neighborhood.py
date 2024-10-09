from utils import *

#### loads the dictionary of neighbors based on range and then returns the neighborhood for a specific consumer/prosumer with the specified range
def findNeighborhood(range, index):
	R = loaddump(join(BIN_FOLDER, "rangedic.bin"))
	return R[range][index]

