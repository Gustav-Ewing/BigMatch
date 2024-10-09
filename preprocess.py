import sys

if(len(sys.argv) == 0):
    sys.argv = ["preprocess.py"]

from optimizer import *

##### how to run ######
##### python3 preprocess.py
##### example ####
#### python3 preprocess.py
#### for now this script just exports the data for c++
opt = Optimizer.opt2221()


pvfile = open("pvdata.txt", "w")
pv = " ".join(str(x) for x in opt.pv)
pvfile.write(pv)
pvfile.close()
batteryfile = open("batterydata.txt", "w")
battery = " ".join(str(x) for x in opt.battery)
batteryfile.write(battery)
batteryfile.close()

##### The file generated from the following is huge so it's ignored
##### consfile = open("consdata.txt", "w")
##### cons = " ".join(str(x) for x in opt.cons)
##### consfile.write(cons)
##### consfile.close()