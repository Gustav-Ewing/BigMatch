import csv
import sys
import re

##### use testresults.txt filename as input


testfile = open(sys.argv[1]).read().split("\n")

resultrows = []

fields = ["test name" , "weights used" , "total weight saved" , "prosumers matched"]
delimiter = " "


for line in testfile:
    row = []
    if(line == ''): break
    line = line.split(" ")
    
    testname = delimiter.join(line[0:6])
    row.append(testname)

    row.append(re.findall(r'\d+', line[11])[0])
    row.append(re.findall(r"\d+\.*\d+", line[15])[0])
    row.append(re.findall(r'\d+', line[20])[0])

    resultrows.append(row)



with open("results.csv", "w") as csvfile:
    write = csv.writer(csvfile)
    write.writerow(fields)
    write.writerows(resultrows)


