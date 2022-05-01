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
os.system("make clean")
os.system("make -j8 debug")
os.system(f"{wuhbtool} NUSspli.rpx NUSspli.wuhb --name=NUSspli --short-name=NUSspli --author=V10lator --icon=meta/menu/iconTex.tga --tv-image=meta/menu/bootTvTex.tga --drc-image=meta/menu/bootDrcTex.tga --content=data")
pathsToCreate = ["zips", "NUStmp/code"]
for path in pathsToCreate:
    os.makedirs(path, exist_ok=True)
shutil.make_archive(f"zips/NUSspli-{version}-Aroma-DEBUG", "zip", ".", "NUSspli.wuhb")
shutil.copytree("meta/menu", "NUStmp/meta")
os.remove("NUStmp/meta/app.xml")
os.remove("NUStmp/meta/cos.xml")
code = ["NUSspli.rpx", "meta/menu/app.xml", "meta/menu/cos.xml"]
for file in code:
    shutil.copy(file, "NUStmp/code")
shutil.copytree("data", "NUStmp/content")
os.system(f"java -jar {nuspacker} -in NUStmp -out NUSspli")
try:
    shutil.make_archive(f"zips/NUSspli-{version}-Channel-DEBUG", "zip", ".", "NUSspli")
    shutil.rmtree("NUSspli")
except:
    print("Failed to create Channel-DEBUG zip")

if not isBeta:
    os.system("make clean")
    os.system("make -j8 release")
    os.system(f"{wuhbtool} NUSspli.rpx NUSspli.wuhb --name=NUSspli --short-name=NUSspli --author=V10lator --icon=meta/menu/iconTex.tga --tv-image=meta/menu/bootTvTex.tga --drc-image=meta/menu/bootDrcTex.tga --content=data")
    shutil.make_archive(f"zips/NUSspli-{version}-Aroma", "zip", ".", "NUSspli.wuhb")
    os.remove("NUStmp/code/NUSspli.rpx")
    shutil.move("NUSspli.rpx", "NUStmp/code")
    os.system(f"java -jar {nuspacker} -in NUStmp -out NUSspli")
    try:
        shutil.make_archive(f"zips/NUSspli-{version}-Channel", "zip", ".", "NUSspli")
        shutil.rmtree("NUSspli")
    except:
        print("Failed to create Channel zip")

shutil.rmtree("NUStmp")
os.system("make clean")
os.system("make HBL=1 -j8 debug")
os.makedirs("NUSspli", exist_ok=True)
hblFiles = ["NUSspli.rpx", "meta/hbl/meta.xml", "meta/hbl/icon.png"]
for file in hblFiles:
    shutil.copy(file, "NUSspli")
shutil.make_archive(f"zips/NUSspli-{version}-HBL-DEBUG", "zip", ".", "NUSspli")
shutil.rmtree("NUSspli")

if not isBeta:
    os.system("make clean")
    os.system("make HBL=1 -j8 release")
    os.makedirs("NUSspli", exist_ok=True)
    for file in hblFiles:
        shutil.copy(file, "NUSspli")
    shutil.make_archive(f"zips/NUSspli-{version}-HBL", "zip", ".", "NUSspli")
    shutil.rmtree("NUSspli")
