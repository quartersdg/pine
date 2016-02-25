#!/usr/bin/python
import json
from pprint import pprint
import sys
import glob;
import os;

AppProjectEntry = """
    <ClCompile Include="{}">
      <PrecompiledHeader Condition="'$(Configuration)|$(Platform)'=='Aplite|Win32'">NotUsing</PrecompiledHeader>
      <AdditionalIncludeDirectories Condition="'$(Configuration)|$(Platform)'=='Aplite|Win32'">../Pebble;{};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PrecompiledHeader Condition="'$(Configuration)|$(Platform)'=='Basalt|Win32'">NotUsing</PrecompiledHeader>
      <AdditionalIncludeDirectories Condition="'$(Configuration)|$(Platform)'=='Basalt|Win32'">../Pebble;{};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PrecompiledHeader Condition="'$(Configuration)|$(Platform)'=='Chalk|Win32'">NotUsing</PrecompiledHeader>
      <AdditionalIncludeDirectories Condition="'$(Configuration)|$(Platform)'=='Chalk|Win32'">../Pebble;{};%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
""";

AppResourceProjectEntry = """
    <ResourceCompile Include="{}">
    </ResourceCompile>
""";

AppProjectFilterEntry = """
    <ClCompile Include="{}">
      <Filter>app</Filter>
    </ClCompile>
""";

AppResourceProjectFilterEntry = """
    <ResourceCompile Include="{}">
      <Filter>app</Filter>
    </ResourceCompile>
""";

appdir = os.path.dirname(sys.argv[1]);
sources = glob.glob(appdir+"/src/*.c");

with open("PINE/PINE.base.vcxproj") as inprj:
    with open("PINE/PINE.vcxproj","w") as outprj:
        for line in inprj:
            if "APP ENTRY LOCATION" in line:
                for f in sources:
                    line = AppProjectEntry.format("../"+f,
			"../"+appdir+"/build/",
			"../"+appdir+"/build/",
			"../"+appdir+"/build/")
                    outprj.write(line)
                    line = AppResourceProjectEntry.format("../"+appdir+"/build/src/"+appdir+".rc")
                    outprj.write(line)
            else:
                outprj.write(line)


with open("PINE/PINE.base.vcxproj.filters") as inprj:
    with open("PINE/PINE.vcxproj.filters","w") as outprj:
        for line in inprj:
            if "APP ENTRY LOCATION" in line:
                for f in sources:
                    line = AppProjectFilterEntry.format("../"+f)
                    outprj.write(line)
                    line = AppResourceProjectFilterEntry.format("../"+appdir+"/build/src/"+appdir+".rc")
                    outprj.write(line)
            else:
                outprj.write(line)

with open(str(sys.argv[1]) + "/appinfo.json") as appinfo_file:
	appinfo = json.load(appinfo_file)

resource_ids_header = """
#pragma once

//
// AUTOGENERATED BY quarter's create_project.py
// DO NOT MODIFY
//

#include <stdint.h>
#include "pebble.h"
typedef enum {
  INVALID_RESOURCE = 0,
  DEFAULT_MENU_ICON = 0, // Friendly synonym for use in `PBL_APP_INFO()` calls
""";
resource_ids_footer = """
} ResourceId;

""";

if not os.path.exists(appdir+"/build/src/"):
	os.makedirs(appdir+"/build/src/")
with open(appdir+"/build/src/resource_ids.auto.h","w") as idhf:
    idhf.write(resource_ids_header);
    resources = appinfo["resources"]
    for key in resources:
        for res in resources[key]:
            idhf.write("RESOURCE_ID_"+res["name"]+",\n");
    idhf.write(resource_ids_footer);

with open(appdir+"/build/src/"+appdir+".rc","w") as rcf:
    resources = appinfo["resources"]
    index = 1
    for key in resources:
        for res in resources[key]:
            rcf.write("#define RESOURCE_ID_{} {}\n".format(res["name"],index));
            rcf.write("RESOURCE_ID_{} RCDATA \"../{}/resources/{}\"\n".format(res["name"],appdir,res["file"]));
            index=index+1;
