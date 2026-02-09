# Discord connector plugin สำหรับ San Andreas Multiplayer (SA:MP)

**ภาษา:** [English](README.md) | ภาษาไทย

| CI | ดาวน์โหลดทั้งหมด | รีลีสล่าสุด |
| :---: | :---: | :---: |
|  [![Build status](https://github.com/GimmickPlus/samp-discord-connector/actions/workflows/build.yml/badge.svg)](https://github.com/GimmickPlus/samp-discord-connector/actions/workflows/build.yml) |  [![All Releases](https://img.shields.io/github/downloads/GimmickPlus/samp-discord-connector/total.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases)  |  [![latest release](https://img.shields.io/github/release/GimmickPlus/samp-discord-connector.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases) <br> [![Github Releases](https://img.shields.io/github/downloads/GimmickPlus/samp-discord-connector/latest/total.svg?maxAge=86400)](https://github.com/GimmickPlus/samp-discord-connector/releases)  |

-------------------------------------------------
**ปลั๊กอินนี้ช่วยให้คุณควบคุม Discord bot ได้จากภายในสคริปต์ PAWN ของคุณ**

**วิธีติดตั้งบน open.mp**
-----------------------------------
1. แตกไฟล์จาก archive ไปยังโฟลเดอร์ที่ต้องการ แล้วคัดลอกไฟล์ในโฟลเดอร์ `plugins` ไปไว้ใน **COMPONENTS** (ถ้าไม่ทำ open.mp จะพยายามโหลดเป็นปลั๊กอิน SA:MP)
2. แก้ไขไฟล์ตั้งค่า (**config.json**) เช่น:
   ```json
      "discord": {
         "bot_token": "MYBOTTOKEN"
      }
    ```
   หรือใช้ environment variable **DCC_BOT_TOKEN** แทนได้ (ห้ามแชร์ token ให้ใครเด็ดขาด)

วิธีติดตั้งบน SA:MP
--------------------------------
1. แตกไฟล์จาก archive ไปไว้ในโฟลเดอร์หลักของ SA:MP server
2. แก้ไขไฟล์ `server.cfg`:
   - Windows: `plugins discord-connector`
   - Linux: `plugins discord-connector.so`
3. เพิ่ม `discord_bot_token YOURDISCORDBOTTOKEN` ใน `server.cfg` หรือกำหนดผ่าน environment variable `DCC_BOT_TOKEN` (ห้ามแชร์ token ให้ใครเด็ดขาด)

Pawn Encoding / ภาษาไทย (Windows-874)
--------------------------------
ถ้าสคริปต์ Pawn ของคุณเป็น encoding Windows-874 (ไทย) ให้ตั้งค่าเพื่อให้ปลั๊กอินแปลงระหว่าง Windows-874 <-> UTF-8:
- SA:MP (`server.cfg`): `discord_pawn_encoding windows-874`
- open.mp (`config.json`): `"discord": { "pawn_encoding": "windows-874" }`
- Environment variable: `DCC_PAWN_ENCODING=windows-874`

Startup / รอการ Initialize
-----------------------------
ค่าเริ่มต้นปลั๊กอินจะ initialize แบบ background (เพื่อไม่ให้ทำให้การเปิดเซิร์ฟช้าลง) หากต้องการให้บล็อกการเริ่มเซิร์ฟเวอร์จนกว่าจะ init ข้อมูล Discord เสร็จ:
- SA:MP (`server.cfg`): `discord_init_block_ms 20000`
- open.mp (`config.json`): `"discord": { "init_block_ms": 20000 }`
- Environment variable: `DCC_INIT_BLOCK_MS=20000`

หากต้องการกำหนดเวลาว่าเมื่อไหร่ถึงจะถือว่า init "timeout" และเริ่ม retry ต่อแบบ background:
- `discord_init_timeout_ms` / `discord.init_timeout_ms` / `DCC_INIT_TIMEOUT_MS`
  ค่าเริ่มต้นคือ `120000` (120 วินาที)

ขึ้น intent error ต้องแก้ยังไง?
---------------
ถ้าขึ้น intent error ให้เข้าไปที่ [Discord developer dashboard](https://discord.com/developers/applications) เลือกบอทของคุณ จากนั้นไปที่ตั้งค่าบอทและเปิด intents ที่ต้องใช้

วิธี Build
---------------
*หมายเหตุ*: ปลั๊กอินต้องเป็นไลบรารี 32-bit ดังนั้นไลบรารีที่เกี่ยวข้องทั้งหมดต้องคอมไพล์แบบ 32-bit และคอมไพเลอร์ต้องรองรับ 32-bit

#### Windows
1. ติดตั้ง C++ compiler ที่ต้องการ
2. ติดตั้ง [CMake](http://www.cmake.org/)
3. ติดตั้ง [Conan](https://conan.io)
4. clone repository แบบ recursive (`git clone --recursive https://...`)
5. สร้างโฟลเดอร์ `build` และรัน CMake ในนั้น
6. build project ที่ generate ขึ้นด้วยคอมไพเลอร์ของคุณ

#### Linux
1. ติดตั้ง C++ compiler ที่ต้องการ
2. ติดตั้ง [CMake](http://www.cmake.org/)
3. ติดตั้ง [Conan](https://conan.io)
4. clone repository แบบ recursive (`git clone --recursive https://...`)
5. สร้างโฟลเดอร์ `build` และรัน CMake ในนั้น (`mkdir build && cd build && cmake ..`)
6. build project ที่ generate ขึ้นด้วยคอมไพเลอร์ของคุณ

