import numpy as np
import pandas as pd
import argparse
parser = argparse.ArgumentParser(
                    prog='run a full livestream',
                    # description='calc output by mpi',
                    epilog='Text at the bottom of help')

parser.add_argument('mpiproc', type=int)
parser.add_argument("--prefix")
args = parser.parse_args()