import sys


#### current method for calling python functions doesn't set argv when it calls them and one of the libraries downstream uses argv[0] for something
#### this simply sets argv[0] to the same value it would have had if you called "python3 preprocess.py ..."
if(len(sys.argv) == 0):
    sys.argv = ["preprocess.py"]

from optimizer import *


def preprocess():
    opt = Optimizer.opt2221()

    prosumerList = []
    consumerList = []
    if len(opt.pv) != len(opt.battery):
        print("error: Malformed data! Different amount of entries in pv and battery lists")
        #### might be better to return a different value here but that might cause issues on the receiver side
        #### it's better to just watch for the above error I think
        return prosumerList, consumerList


    for i in range(len(opt.pv)):
        if opt.pv[i] != 0:
            prosumerList.append(i)
        elif opt.battery[i] != 0:
            prosumerList.append(i)
        else:
            consumerList.append(i)

    return prosumerList, consumerList

def getLength():
    opt = Optimizer.opt2221()
    return len(opt.pv)
