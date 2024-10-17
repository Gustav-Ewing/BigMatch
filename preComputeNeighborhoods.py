from utils import *

def start():
	global neighborhoods
	neighborhoods = {}


#### finds the neighborhoods of all prosumers and consumers based on a dictionary of edges
def findNeighborhoods(loadFile, saveFile):
	#neighborhoods = {}
	costdict = loaddump(join("bin/costs/", loadFile))
	keys = costdict.keys()
	prosumerNeighborhoods = {}

	#### this constructs the forward dictionary
	for key in keys:

		#### we only want to look at edges that connects a prosumer to a consumer
		if type(key) is int:
			continue
		#### Recieved a tuple with length 1 once somehow so len > 2 isnt enough
		if len(key) != 2:
			continue

		#### if the prosumer has no neighborhood yet then we create it
		if key[0] not in prosumerNeighborhoods.keys():
			neighborhood = []
			prosumerNeighborhoods[key[0]] = neighborhood

		#### adding the consumer to the prosumers neighborhood
		prosumerNeighborhoods[key[0]].append(key[1])

	prosumerList = list(prosumerNeighborhoods.keys())

	#### this makes the reverse dictionary using the forward dictionary
	consumersnNeighborhoods = {}
	prosumers = prosumerNeighborhoods.keys()
	for prosumer in prosumers:
		prosumerNeighborhood = prosumerNeighborhoods[prosumer]
		for consumer in prosumerNeighborhood:

			#### if the consumer has no neighborhood yet then we create it
			if consumer not in consumersnNeighborhoods.keys():
				neighborhood = []
				consumersnNeighborhoods[consumer] = neighborhood

			#### adding the prosumer to the consumers neighborhood
			consumersnNeighborhoods[consumer].append(prosumer)

	consumerList = list(consumersnNeighborhoods.keys())

	#### merges the forward and reverse dictionaroes so that we can lookup both consumer and prosumers
	# global neighborhoods
	global neighborhoods
	neighborhoods = prosumerNeighborhoods | consumersnNeighborhoods
	savedump(neighborhoods, join("bin/dumps/", saveFile))

	return prosumerList, consumerList

#### precalculates all the neighbors based on precomputed costs and saves them all to disk
#### it's actually very fast to compute findNeighborhoods() even for 100k so there is no real need for this
#### keeping this around for future use but for it is better to call findNeighborhoods directly while in the precompute stage
def precalculateNeighborhoods():
	files = ["20","100","1k","2k","4k-15","6k-15","12k-15","25k-15","50k-15","100k-8"]
	for fil in files:
		findNeighborhoods(f"costs{fil}.bin", f"dumps{fil}.bin")
	return "finished dumping all precomputed neighborhoods to bin/dumps/"


# this global isnt working so have to load it every time instead
def findNeighborhood(sumer):
	#neighborhoods = loaddump(join("bin/dumps/", "costs100.bin"))
	return neighborhoods[sumer]

