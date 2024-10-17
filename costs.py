from utils import *

#### finds all neighborhoods in a dictionary
#### very expensive on large dictionaries we should precompute this
def findNeighborhoods(file, savefile, threshhold):
	global costdict
	costdict = loaddump(join("bin/costs/", file))
	keys = costdict.keys()
	prosumerkeys = []
	#i = 0
	for key in keys:
		if type(key) is int:
			continue
		i = threshhold
		while(i>-1):
		#while(i<len(keys)):
			if key[0] == i:
				prosumerkeys.append(key)
				break
			else:
				i = i-1
				#i=i+1

	savedump(prosumerkeys, join("bin/dumps/",savefile))

	return prosumerkeys


def findNeighborhood(prosumerkeys, prosumer):
	neighbors = []
	for key in prosumerkeys:
		if key[0] == prosumer:
			neighbors.append(key[1])

			#### if a neighbor only appears with tuple index > 1 the following is needed
			#tupleLen = len(key)
			#for i in range(1,tupleLen):
				#neighbors.append(key[i])

	#### removes duplicate neighbors
	return list(set(neighbors))

def getCost(sumer):
	return costdict[sumer]

