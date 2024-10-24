


import subprocess

open('testresult.txt', 'w').close()

datasets = ["20.bin","100.bin","1k.bin","2k.bin","4k-15.bin","6k-15.bin","12k-15.bin","25k-15.bin","50k-15.bin","100k-8.bin"]
#### make sure that the actual binary files associated with the datasets used are named correctly and actually exist
for arg1 in [0]:
    for arg2 in [0,1,3,7]:
        #### slice or select to test specific datasets
        for arg3 in datasets:
            for arg4 in [0,1]:
                for arg5 in [0,1]:
                    subprocess.run(["build/Matching", f"{arg1}", f"{arg2}", f"{arg3}", f"{arg4}", f"{arg5}"])
