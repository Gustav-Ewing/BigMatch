import sys
from optimizer import *

##### how to run ######
##### python3 runopt.py groupsize index1 index2 etc..
##### example ####
#### python3 runopt.py 3 1000 1200 805


opt = Optimizer.opt2221()

pvfile = open("pvdata.txt", "w")
pv = " ".join(str(x) for x in opt.pv)
pvfile.write(pv)
pvfile.close()
batteryfile = open("batterydata.txt", "w")
battery = " ".join(str(x) for x in opt.battery)
batteryfile.write(battery)
batteryfile.close()
consfile = open("consdata.txt", "w")
cons = " ".join(str(x) for x in opt.battery)
consfile.write(cons)
consfile.close()


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