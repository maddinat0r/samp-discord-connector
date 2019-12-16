# Discord connector plugin for San Andreas Multiplayer (SA:MP)

| AppVeyor CI | Total downloads | Latest release |
| :---: | :---: | :---: |
|  [![Build status](https://ci.appveyor.com/api/projects/status/hr53e3q8etb06xta/branch/master?svg=true)](https://ci.appveyor.com/project/maddinat0r/samp-discord-connector/branch/master)  |  [![All Releases](https://img.shields.io/github/downloads/maddinat0r/samp-discord-connector/total.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases)  |  [![latest release](https://img.shields.io/github/release/maddinat0r/samp-discord-connector.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases) <br> [![Github Releases](https://img.shields.io/github/downloads/maddinat0r/samp-discord-connector/latest/total.svg?maxAge=86400)](https://github.com/maddinat0r/samp-discord-connector/releases)  |  
-------------------------------------------------
**This plugin allows you to control a Discord bot from within your PAWN script.**

How to install
--------------
1. Extract the content of the downloaded archive into the root directory of your SA-MP server.
2. Edit the server configuration (*server.cfg*) as follows:
   - Windows: `plugins discord-connector`
   - Linux: `plugins discord-connector.so`
3. Add `discord_bot_token YOURDISCORDBOTTOKEN` to your *server.cfg* file (__never share your bot token with anyone!__)

Build instruction
---------------
*Note*: The plugin has to be a 32-bit library; that means all required libraries have to be compiled in 32-bit and the compiler has to support 32-bit.
#### Windows
1. install a C++ compiler of your choice
3. install the [Boost libraries (version 1.69)](http://www.boost.org/users/download/)
4. install [CMake](http://www.cmake.org/)
5. clone this repository recursively (`git clone --recursive https://...`)
6. download the full log-core package [here](https://github.com/maddinat0r/samp-log-core/releases/latest)
7. create a folder named `build` and execute CMake in there
8. build the generated project files with your C++ compiler

#### Linux
1. install a C++ compiler of your choice
3. install the [Boost libraries (version 1.69)](http://www.boost.org/users/download/)
4. install [CMake](http://www.cmake.org/)
5. clone this repository recursively (`git clone --recursive https://...`)
6. download the full log-core package [here](https://github.com/maddinat0r/samp-log-core/releases/latest)
7. create a folder named `build` and execute CMake in there (`mkdir build && cd build && ccmake ..`)
8. build the generated project files with your C++ compiler
