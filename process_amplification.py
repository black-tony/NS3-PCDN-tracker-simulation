import numpy as np
import pandas as pd
import argparse

parser = argparse.ArgumentParser(
                    prog='processAmplification',
                    description='calc output by mpi',
                    epilog='Text at the bottom of help')

parser.add_argument('mpiproc', type=int)
parser.add_argument("--prefix")
args = parser.parse_args()
DEFAULT_OUTPUT = "output/MytestCountsMesh-part-{}"
output_fileformat = DEFAULT_OUTPUT
# print(args.mpiproc)
if args.prefix:
    output_fileformat = args.prefix
    # print(args.prefix.format("222"))
    
ans = {}
max_time = 0
for i in range(args.mpiproc):
    now_filename = output_fileformat.format(i)
    now_filename = now_filename + ".txt"
    with open(now_filename, "r") as f:
        for line in f:
            linesplit = line.split()
            if(len(linesplit) == 3):
                no_unit = linesplit[2].removesuffix("MB")
                now_time = linesplit[0].removesuffix("s")
                max_time = max(max_time, int(now_time))
                # print(no_unit)
                if linesplit[0] not in ans.keys():
                    ans[linesplit[0]] = {"PCDN" : 0, "CDN" : 0, "CLIENT" : 0}
                ans[linesplit[0]][linesplit[1]] += float(no_unit)
            # print(line.split())

for index, value in ans[f"{max_time}s"].items():
    print(f"{index} : {value}MB")
print(f"amplification = {ans[f'{max_time}s']['PCDN']} / {ans[f'{max_time}s']['CDN']} = {ans[f'{max_time}s']['PCDN'] / ans[f'{max_time}s']['CDN']}")