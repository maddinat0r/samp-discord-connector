#Discord connector plugin for San Andreas Multiplayer (SA:MP)
| Travis CI | AppVeyor | Total downloads | Latest release |
| :---: | :---: | :---: | :---: |
|  [![Build Status](https://travis-ci.org/maddinat0r/samp-discord-connector.svg?branch=master)](https://travis-ci.org/maddinat0r/samp-discord-connector)   |  [![Build status](https://ci.appveyor.com/api/projects/status/hr53e3q8etb06xta/branch/master?svg=true)](https://ci.appveyor.com/project/maddinat0r/samp-discord-connector/branch/master)  |  [![All Releases](https://img.shields.io/github/downloads/maddinat0r/samp-discord-connector/total.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases)  |  [![latest release](https://img.shields.io/github/release/maddinat0r/samp-discord-connector.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases) <br> [![Github Releases](https://img.shields.io/github/downloads/maddinat0r/samp-discord-connector/latest/total.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases)  |  
-------------------------------------------------
**This plugin allows you to control a Discord bot from within your PAWN script.**

How to install
--------------
1. Extract the content of the downloaded archive into the root directory of your SA-MP server.
2. Edit the server configuration (*server.cfg*) as follows:
   - Windows: `plugins discord-connector`
   - Linux: `plugins discord-connector.so`

F.A.Q.
------
Q: *I get a* `version GLIBCXX_3.4.15' not found` *error (or similar). How can I solve this?*  
A: Update your system. If that still didn't work, you'll need to upgrade your Linux distribution to a version which provides the gcc 4.9 (or higher) compiler.

Q: *The plugin fails to load on Windows, how can I fix this?*  
A: You have to install these Microsoft C++ redistributables. You'll need the x86/32bit downloads.
   - [2010 (x86)](http://www.microsoft.com/en-us/download/details.aspx?id=5555)
   - [2010 SP1 (x86)](http://www.microsoft.com/en-us/download/details.aspx?id=8328)
   - [2012 (x86)](http://www.microsoft.com/en-us/download/details.aspx?id=30679)
   - [2015 (x86)](https://www.microsoft.com/en-US/download/details.aspx?id=48145)  

Q: *I'm not on Windows 10 and the plugin still fails to load after installing all the redistributables. Is there a solution for this?*  
A: Download the [universal Windows CRT](https://www.microsoft.com/en-US/download/details.aspx?id=48234). Requirements for this:
 - Windows 8.1 and Windows Server 2012 R2: [KB2919355](https://support.microsoft.com/en-us/kb/2919355)  
 - Windows 7 and Windows Server 2008 R2: [Service Pack 1](https://support.microsoft.com/en-us/kb/976932)  
 - Windows Vista and Windows Server 2008: [Service Pack 2](https://support.microsoft.com/en-us/kb/948465)  

Build instruction
---------------
*Note*: The plugin has to be a 32-bit library; that means all required libraries have to be compiled in 32-bit and the compiler has to support 32-bit.
#### Windows
1. install a C++ compiler of your choice
3. install the [Boost libraries (version 1.62)](http://www.boost.org/users/download/)
4. install [CMake](http://www.cmake.org/)
5. clone this repository recursively
6. create a folder named `build` and execute CMake in there
7. build the generated project files with your C++ compiler

#### Linux
1. install a C++ compiler of your choice
3. install the [Boost libraries (version 1.62)](http://www.boost.org/users/download/)
4. install [CMake](http://www.cmake.org/)
5. clone this repository recursively
6. create a folder named `build` and execute CMake in there (`mkdir build && cd build && cmake ..`)
7. build the generated project files with your C++ compiler
