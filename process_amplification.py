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
for i in range(args.mpiproc):
    now_filename = output_fileformat.format(i)
    now_filename = now_filename + ".txt"
    with open(now_filename, "r") as f:
        for line in f:
            linesplit = line.split()
            if(len(linesplit) == 2):
                no_unit = linesplit[1].removesuffix("MB")
                # print(no_unit)
                ans[linesplit[0]] = float(no_unit)
            # print(line.split())

for index, value in ans.items():
    print(f"{index} : {value}MB")
print(f"amplification = {ans['PCDN']} / {ans['CDN']} = {ans['PCDN'] / ans['CDN']}")