from utils import *

#### this a mirror of runopt.py
#### however, it uses precomputed values loaded into the dictionary costs instead of computing them online


def loadCost(loadFile):
    global costs
    costs = loaddump(join("bin/costs/",loadFile))


def runOptimize(indexList):
    # costs = loaddump(join("bin/costs/", "costs100.bin"))
    indexTuple = tuple(indexList)
    totalcost  = 0
    groupcost  = costs[indexTuple]
    for index in indexTuple:
         totalcost += costs[index]

    savings = totalcost - groupcost
    if abs(savings) < 0.0000000001:
        savings = 0

    return savings

# this one is awkward because these files have a different naming standard than the rest
def getLength(loadFile):
    opt = loaddump(join("bin/opts/", loadFile))
    return len(opt.pv)

