import ujson
from threading import Thread
import requests
import os
import time

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-t", "--roundlen", type=int, default=10000, help="Length of a round in milliseconds")
parser.add_argument("-d", "--directory", default="dumps", help="Directory where to read the roundi.txt files")
parser.add_argument("--skip", default=0, type=int, help="First round to start with")
args = parser.parse_args()

def forward(filename):
    print("Animating "+filename)
    try:
        with open(filename, "r") as text_file:
            r = requests.post("http://localhost:8081", data=text_file)
    except Exception as e:
        print("Error on sending data to visualize.py: ", e)
   
round = int(args.skip)
pathprefix = args.directory
if pathprefix.endswith("/"):
    pathprefix = pathprefix[:-1]
pathprefix = pathprefix + "/round"
pathsuffix = ".txt"
while os.path.isfile(pathprefix+str(round)+pathsuffix):
    forward(pathprefix+str(round)+pathsuffix)
    time.sleep(args.roundlen/1000)
    round += 1

if round == 0:
    print("No files found")