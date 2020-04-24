# WUPDownloader
Download installable content from the NUS, directly on the WiiU SD Card

This app doesn't support piracy. Only download games that you had bought before!

# Features
- Download titles from Nintendo's servers (NUS) on the SD Card (SD:/install/)
- Create fake tickets at the start of a download if not found on the NUS
- Shows the download speed for any title
- Also, generate fake tickets for other downloaded titles on <SD:/install/> folder (To fix missing ticket downloads)
- WiiU Software Keyboard library support (SWKBD)
- Can download System/vWii titles
- Complete HOME Menu support for channel version
- Downloaded titles can be installed with WUPInstaller
- Download app hashes support (.h3)
- Custom folder names for downloaded titles

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

# Changelog
### 1.0 [22 Sep. 2019]
- Initial release

### 1.1 [6 Oct. 2019]
- Added download speed
- (Channel ver.) Fixed crash while trying to exit to HOME Menu
- (Channel ver.) Now you can place the app on the background process
- Fixed a bug where you cannot write an space on SWKBD
- Updated download size to be showed as "B", "Kb" or "Mb"
- Some other fixes

### 1.2 [24 Apr. 2020]
This app was made some months ago so I had to write new (and better) code on top of old code. I updated it only for fun. Feel free to make it better if you want, because i dont have so much desire to continue with this
- Fixed issue #1 by moving graphic processes to another thread. This made:
  - Download speeds now are much faster than before
- Fixed issue #2 by changing how the new ticket filename is calculated
- Now log functions don't send debug data unless you compile it with DEBUG tag defined in "debug.hpp"

# Building
1. On Linux/WSL, download and install WUT from devkitPRO's github [https://github.com/devkitPro/wut]
2. Follow the instructions to install it
3. Unzip the contents of the repo at any folder on your system
4. Open the terminal and run:
```
cd wupdownloader-master
make
```
5.- If everything goes fine, you will have the resulting file "WUPDownloader.rpx"

# Info
Feel free to fork or use this project, and don't forget to give credits!
Contact: pokes303dev@gmail.com
