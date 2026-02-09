# Discord connector plugin for San Andreas Multiplayer (SA:MP)

**Language:** English | [ภาษาไทย](README.th.md)

| CI | Total downloads | Latest release |
| :---: | :---: | :---: |
|  [![Build status](https://github.com/GimmickPlus/samp-discord-connector/actions/workflows/build.yml/badge.svg)](https://github.com/GimmickPlus/samp-discord-connector/actions/workflows/build.yml) |  [![All Releases](https://img.shields.io/github/downloads/GimmickPlus/samp-discord-connector/total.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases)  |  [![latest release](https://img.shields.io/github/release/GimmickPlus/samp-discord-connector.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases) <br> [![Github Releases](https://img.shields.io/github/downloads/GimmickPlus/samp-discord-connector/latest/total.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases)  |  
-------------------------------------------------
**This plugin allows you to control a Discord bot from within your PAWN script.**

**How to install on an open.mp server**
-----------------------------------
1. Extract the contents of the archive to a directory, copy the file(s) in plugins into **COMPONENTS** if you do not do this, it will try to load as a SA:MP plugin instead.
2. Edit your configuration file (**config.json**) as follows:
   ```json
      "discord": {
         "bot_token": "MYBOTTOKEN"
      }
    ```
   Alternatively you can use the enviromental variable **DCC_BOT_TOKEN** to set the token instead. **DO NOT SHARE YOUR TOKEN WITH ANYONE**

How to install on a SA:MP server
--------------------------------
1. Extract the content of the downloaded archive into the root directory of your SA-MP server.
2. Edit the server configuration (*server.cfg*) as follows:
   - Windows: `plugins discord-connector`
   - Linux: `plugins discord-connector.so`
3. Add `discord_bot_token YOURDISCORDBOTTOKEN` to your *server.cfg* file, or set it in the environment variable `DCC_BOT_TOKEN` (__never share your bot token with anyone!__)

Pawn Encoding / Thai (Windows-874)
--------------------------------
If your Pawn scripts are encoded as Windows-874 (Thai), configure the plugin to convert between Windows-874 <-> UTF-8:
- SA:MP (`server.cfg`): `discord_pawn_encoding windows-874`
- open.mp (`config.json`): `"discord": { "pawn_encoding": "windows-874" }`
- Environment variable: `DCC_PAWN_ENCODING=windows-874`

Startup / Initialization Wait
-----------------------------
By default the plugin initializes in the background (so it does not delay server startup). If you want to block server startup until Discord data is initialized:
- SA:MP (`server.cfg`): `discord_init_block_ms 20000`
- open.mp (`config.json`): `"discord": { "init_block_ms": 20000 }`
- Environment variable: `DCC_INIT_BLOCK_MS=20000`

If you want to control when the plugin considers initialization "timed out" and starts retrying in the background:
- `discord_init_timeout_ms` / `discord.init_timeout_ms` / `DCC_INIT_TIMEOUT_MS`
  Default is `120000` (120 seconds).

I am getting a intent error, how do I fix it?
---------------
If you're getting an intent error, you need to go to the [discord developer dashboard](https://discord.com/developers/applications) and select your bot.
Then, you need to go to your bot settings and activate your intents.

Build instruction
---------------
*Note*: The plugin has to be a 32-bit library; that means all required libraries have to be compiled in 32-bit and the compiler has to support 32-bit.
#### Windows
1. install a C++ compiler of your choice
2. install [CMake](http://www.cmake.org/)
3. install [Conan](https://conan.io)
4. clone this repository recursively (`git clone --recursive https://...`)
5. create a folder named `build` and execute CMake in there
6. build the generated project files with your C++ compiler

#### Linux
1. install a C++ compiler of your choice
2. install [CMake](http://www.cmake.org/)
3. install [Conan](https://conan.io)
4. clone this repository recursively (`git clone --recursive https://...`)
5. create a folder named `build` and execute CMake in there (`mkdir build && cd build && cmake ..`)
6. build the generated project files with your C++ compiler
