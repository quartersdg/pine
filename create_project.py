#!/usr/bin/python
import json
from pprint import pprint
import sys

with open(str(sys.argv[1]) + "/appinfo.json") as appinfo_file:
	appinfo = json.load(appinfo_file)

resources = appinfo["resources"]
for key in resources:
    for res in resources[key]:
        print(res["name"])
        print(res["file"])
