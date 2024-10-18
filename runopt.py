import sys
#### current method for calling python functions doesn't set argv when it calls them and one of the libraries downstream uses argv[0] for something
#### this simply sets argv[0] to the same value it would have had if you called "python3 runopt.py ..."
if(len(sys.argv) == 0):
    sys.argv = ["runopt.py"]

from optimizer import *

##### how to run ######
##### python3 runopt.py groupsize index1 index2 etc..
##### example ####
#### python3 runopt.py 3 1000 1200 805

def runOptimize(groupSize, indexlist):

    groupcost = opt.subset(indexlist).aggregate_all().optimize()
    totalcost = opt.subset(indexlist).optimize_all()

    savings = totalcost - groupcost

    return savings

def loadOpt():
    global opt
    opt = Optimizer.opt2221()

