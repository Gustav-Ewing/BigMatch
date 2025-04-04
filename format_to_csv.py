import csv
import sys
import re

##### use testresults.txt filename as input


testfile = open(sys.argv[1]).read().split("\n")

resultrows = []

fields = ["test name" , "weights used" , "total weight saved" , "prosumers matched", "execution time (seconds)"]
delimiter = " "


for line in testfile:
    row = []
    if(line == ''): break
    line = line.split(" ")
    
    testname = delimiter.join(line[0:6])
    row.append(testname)

    print(line)
    row.append(re.findall(r'\d+', line[12])[0])
    row.append(re.findall(r"\d+\.*\d+", line[17])[0])
    row.append(re.findall(r'\d+', line[23])[0])
    row.append(re.findall(r'\d+', line[6])[0])

    resultrows.append(row)



with open("results.csv", "w") as csvfile:
    write = csv.writer(csvfile)
    write.writerow(fields)
    write.writerows(resultrows)


