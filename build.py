#!/bin/env python

import xml.etree.cElementTree as ET
import os
import shutil

nuspacker = "../nuspacker/NUSPacker.jar"    # Set path to NUSPacker.jar here. will be downloaded if empty
wuhbtool = ""                               # Set path to wuhbtool. Will use the one from PATH if empty
ForceRelease = False                        # set to True to force release builds even if we'e building ALPHA/BETA

# Don't edit below this line

def checkAndDeleteFile(file):
    if os.path.exists(file):
        print(f"Deleting {file}")
        os.remove(file)

def checkAndDeleteDir(dir):
    if os.path.exists(dir):
        print(f"Deleting {dir}")
        shutil.rmtree(dir)

version = ET.ElementTree(file="meta/hbl/meta.xml").getroot().findtext("version")

isBeta = False
if ForceRelease or version.find("BETA") != -1 or version.find("ALPHA") != -1:
    isBeta = True

if len(wuhbtool) == 0:
    wuhbtool = "wuhbtool"

checkAndDeleteFile("NUSspli.wuhb")
checkAndDeleteDir("NUSspli")
checkAndDeleteDir("NUStmp")
checkAndDeleteDir("out")

editionList = ["-DEBUG", ""]
extList = [".rpx", ".zip", ".wuhb"]
pkgList = ["Aroma", "HBL", "Channel"]
for edition in editionList:
    for ext in extList:  
        checkAndDeleteFile(f"NUSspli-{version}{edition}{ext}")
for edition in editionList:
    for ext in extList:
        for pkg in pkgList:
            checkAndDeleteFile(f"zips/NUSspli-{version}-{pkg}{edition}{ext}")

pathsToCreate = ["out/Aroma-DEBUG", "out/Channel-DEBUG", "out/HBL-DEBUG", "zips", "NUStmp/code"]
for path in pathsToCreate:
    os.makedirs(path, exist_ok=True)
os.system("make clean")
os.system("make -j8 debug")
os.system(f"{wuhbtool} NUSspli.rpx out/Aroma-DEBUG/NUSspli.wuhb --name=NUSspli --short-name=NUSspli --author=V10lator --icon=meta/menu/iconTex.tga --tv-image=meta/menu/bootTvTex.tga --drc-image=meta/menu/bootDrcTex.tga --content=data")
shutil.make_archive(f"zips/NUSspli-{version}-Aroma-DEBUG", "zip", "out/Aroma-DEBUG", ".")
shutil.copytree("meta/menu", "NUStmp/meta")
os.remove("NUStmp/meta/app.xml")
os.remove("NUStmp/meta/cos.xml")
code = ["NUSspli.rpx", "meta/menu/app.xml", "meta/menu/cos.xml"]
for file in code:
    shutil.copy(file, "NUStmp/code")
shutil.copytree("data", "NUStmp/content")
os.system(f"java -jar {nuspacker} -in NUStmp -out out/Channel-DEBUG/NUSspli")
shutil.make_archive(f"zips/NUSspli-{version}-Channel-DEBUG", "zip", "out/Channel-DEBUG", ".")

if not isBeta:
    os.system("make clean")
    os.system("make -j8 release")
    os.makedirs("out/Aroma", exist_ok=True)
    os.system(f"{wuhbtool} NUSspli.rpx out/Aroma/NUSspli.wuhb --name=NUSspli --short-name=NUSspli --author=V10lator --icon=meta/menu/iconTex.tga --tv-image=meta/menu/bootTvTex.tga --drc-image=meta/menu/bootDrcTex.tga --content=data")
    shutil.make_archive(f"zips/NUSspli-{version}-Aroma", "zip", "out/Aroma", ".")
    os.remove("NUStmp/code/NUSspli.rpx")
    shutil.move("NUSspli.rpx", "NUStmp/code")
    os.makedirs("out/Channel", exist_ok=True)
    os.system(f"java -jar {nuspacker} -in NUStmp -out out/Channel/NUSspli")
    shutil.make_archive(f"zips/NUSspli-{version}-Channel", "zip", "out/Channel", ".")

shutil.rmtree("NUStmp")
os.system("make clean")
os.system("make HBL=1 -j8 debug")
os.makedirs("out/HBL-DEBUG/NUSspli", exist_ok=True)
hblFiles = ["NUSspli.rpx", "meta/hbl/meta.xml", "meta/hbl/icon.png"]
for file in hblFiles:
    shutil.copy(file, "out/HBL-DEBUG/NUSspli")
shutil.make_archive(f"zips/NUSspli-{version}-HBL-DEBUG", "zip", "out/HBL-DEBUG", ".")

if not isBeta:
    os.system("make clean")
    os.system("make HBL=1 -j8 release")
    os.makedirs("out/HBL/NUSspli", exist_ok=True)
    for file in hblFiles:
        shutil.copy(file, "out/HBL/NUSspli")
    shutil.make_archive(f"zips/NUSspli-{version}-HBL", "zip", "out/HBL", ".")
