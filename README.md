# WUPDownloader
Download installable content from the NUS, directly on the WiiU SD Card

Please, don't do piracy with this app! Only download games that you had bought before

# Features
- Download titles from Nintendo's servers (NUS) on the SD Card (SD:/install/)
- Create fake tickets at the start of a download if not found on the NUS
- Also, generate fake tickets for other downloaded titles on <SD:/install/> folder (To fix missing ticket downloads)
- WiiU Software Keyboard library support (SWKBD)
- Downloaded titles can be installed through WUPInstaller
- Download file hashes support (.h3)
- Custom folder names

# Usage
To download a title, search on a Title Database for a title ID (Ex: WiiUBrew's database)\
To create a fake ticket, you will need the title ID and the encryption key (Avaible on 'That title key site')

To install the app, download and unzip the contents of the latest release [https://github.com/Pokes303/WUPDownloader/releases] and depending on how you will run the app, follow the next steps:

### Homebrew Launcher
Move the folder to (SD:/wiiu/apps/)
Run the app from HBL through Haxchi, Browserhax or any exploit you want

### Home Menu
Move the folder to (SD:/install/) and install it with WUPInstaller\
Run it from the HOME Menu (With a CFW)

# Building
1. On Linux/WSL, download and install WUT from devkitPRO's github [https://github.com/devkitPro/wut]
2. Follow the instructions to install it
3. Unzip the contents of the repo at any folder on your system
4. Open the terminal and run:
```
cd drc-test-master
make
```
5.- If everything goes fine, you will have the resulting file "WUPDownloader.rpx"

# Info
Feel free to fork or use this project, and don't forget to give credits!

-> Pokes303
