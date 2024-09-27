import sys
from optimizer import *

##### how to run ######
##### python3 runopt.py groupsize index1 index2 etc..
##### example ####
#### python3 runopt.py 3 1000 1200 805

opt = Optimizer.opt2221()
indexlist = []
groupsize = int(sys.argv[1])

for i in range(groupsize) :
    indexlist.append(int(sys.argv[i+2]))


groupcost = opt.subset(indexlist).aggregate_all().optimize()
totalcost = opt.subset(indexlist).optimize_all()

savings = totalcost - groupcost


print("Aggregate all cost: ", groupcost)
print("Optimize all cost: ", totalcost)
print("Cost savings: ", savings)
