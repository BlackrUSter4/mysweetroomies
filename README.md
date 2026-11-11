# My Sweet Roomies! — revival server

A reconstruction of the server for **My Sweet Roomies!** (`jp.mfapps.novel.roomies`, v1.4.6), the English
edition of Fuji TV's 「ようこそ！ファミーユへ」 (*Welcome to Famille!*). The game ran from 2014 until its servers
were shut down on 13 November 2019. The app itself still works, but it gets everything — the home screen, menus
and every story — from the server, so it has been unplayable since.

This project rebuilds that server from the decompiled app, archived pages and playthrough videos, and makes the
whole game playable again: on your PC with the original app, or fully offline with a single APK.

## Getting the offline APK

No prebuilt APK is published; it contains the original game, so it isn't distributed here. Build it yourself
from your own copy of the original 1.4.6 APK ([Building the offline APK](#building-the-offline-apk)). The result
(about 1 GB) is the game with this server built in: no PC, no internet, no root. It installs as
**"Roomies! Offline"** next to the original app if you have it, and runs on 32-bit and 64-bit-only phones.

On first launch Android shows a "built for an older version of Android" notice and a permission screen, as it
does for the original app. Tap **OK** and **Continue**.

## What works

- **All six routes, start to finish:** the Prologue, every chapter of Ren, Aki, Mitsuka, Aoi, Sayaka and Misaki
  (Misaki unlocks after the other five, as in the original), and both epilogues of each route. About 37,000 lines.
- **The story engine** of the original app, with a picture behind every line.
- **Home screen** with all six heroines, touch scenes for three of them and voiced chat lines for all six.
- **Energy, crystals, login bonuses and streaks, tickets, the Shop, Memory and the Memory Pass**, following the
  original rules.
- **Dress Up:** the outfit machine (gacha) with 145 outfits, plus fuller "card" versions of 20 of them; 8 face
  expressions per heroine; 13 backgrounds.
- **Photo window** with the recovered login-panel and epilogue pictures.
- **23 of the 56 secret stories** and parts of **5 events**.
- **Your own name:** Menu → More → Name (also asked once after the Prologue). It replaces "William" in the stories.
- Purchases are free: the crystal shop just grants the crystals.

## What isn't working or is incomplete

- **Secret stories:** 33 of 56 have no text, so they play a placeholder. Missing: Ren 6–10, Aki 4–10, Aoi 3–10,
  Misaki 4–10. Mitsuka's 1–10 are intentionally not included (she is a high-school student).
- **Events:** most event stories and their pictures were never recorded anywhere. What exists: the hot-spring
  event (Sayaka, Aoi), the Christmas event (Ren), Seaside Cinderella, A Summer Together With Her, and parts of
  the Edo event. Some of these were translated from the Taiwanese edition.
- **Story pictures are video frames.** The original art was served by the game's servers and is lost, so each
  line shows the frame of a playthrough video it was read from (upscaled). Character sprites are baked into
  those frames; there is no separate sprite layer.
- **Story text was transcribed from videos**, then proofread and checked twice against the frames. Expect the odd
  typo or punctuation slip, and an occasional skipped line where the player tapped through.
- **Outfits:** about 180 existed; 145 are in (4–7 missing per heroine). Most come from home-screen screenshots,
  so they show her from the chest up. Item names are descriptive, not the official ones.
- **Backgrounds:** Sayaka's room, the morning balcony, the sunset bridge and the cherry blossoms are locked
  (no picture of them exists).
- **Not known and guessed:** gacha rates, the official English wording of 57 chat lines and of a few secret-story
  titles (Aoi 5, Misaki 1–6), when Aoi's secret story 5 unlocks, and whether the Prologue cost energy.
- **Push notifications, Google sign-in, real payments and ads** are not implemented.
- **Fast tapping** during a story can crash the original story engine (a bug in the app, not the server).

## Running the server on a PC

```
cd server
pip install -r requirements.txt
python app.py                 # listens on port 80
python -m pytest test_server.py
```

Open `http://localhost/?dev=1` to try it in a browser. To use the original app, point
`android.roomies.novel.mfapps.jp` at your PC (a hosts entry on a rooted device or emulator, or a proxy that maps
the host). See [server/README.md](server/README.md) for the details.

The story pictures and the soundtrack are not in this repository (see
[What isn't in this repository](#what-isnt-in-this-repository)). Without the pictures the stories play on a plain
background and a few tests that check story pictures fail; without the soundtrack they play without music.

## Building the offline APK

`offline/` holds the in-app server (`java/`, a port of `server/app.py` that runs inside the app) and the build
scripts. The scripts are written for Windows and look for their tools in fixed places.

**You need:**

| What | Where the scripts look |
|---|---|
| The original **My Sweet Roomies! 1.4.6** APK (your own copy) | decoded into `offline/decoded/` (below) |
| Python 3 with `opencv-python` and `psutil` | on `PATH` |
| JDK 21 | `C:\Program Files\Java\jdk-21` |
| Android SDK: build-tools **34.0.0** and platform **android-33** | `%LOCALAPPDATA%\Android\Sdk` |
| Apktool **2.10.0** | `offline/tools/apktool.jar` |
| *(optional)* the story pictures | `server/data/static/img/resource/` |
| *(optional)* the official soundtrack and ffmpeg | see step 2 |

**1. Decode the original APK** (once):

```
cd offline
java -jar tools/apktool.jar d -f -o decoded "path\to\My Sweet Roomies 1.4.6.apk"
```

**2. Add the soundtrack** (optional, once). Get the official *My Sweet Roomies! (Original Soundtrack)*, 21 MP3s
named like `Aki Theme1 - My Sweet Roomies!.mp3`, then convert them to the engine's format (needs `ffmpeg` on
`PATH`):

```
python convert_ost.py "path\to\soundtrack folder"
```

**3. Build** the APK, which runs on 32-bit phones and on 64-bit phones that can still run 32-bit code:

```
python export_data.py      # story list, transcripts and tables, exported from server/app.py
python export_static.py    # pictures to WebP (7.6 GB -> about 1 GB)
python build_apk.py        # patch, add the server, align and sign -> MySweetRoomies_offline.apk
```

`build_apk.py` creates its own signing key (`offline.keystore`) on first run. Android won't install a build over
one signed with another key, so uninstall the old one first. `--debug` builds `MySweetRoomies_offline_debug.apk`
with logging and `android:debuggable` on.

### The 64-bit APK (phones without 32-bit support)

The original game engine (`lib/armeabi/libotome.so`) is 32-bit only, so it doesn't start on phones that dropped
32-bit support (Snapdragon 8 Elite class, Pixel 7 and later). `offline/native64/` is a from-scratch arm64 rewrite
of it. Build it, then add it to the APK with `--arm64`. The APK keeps the original for 32-bit phones; every phone
that can run the 64-bit one uses it. **It hasn't been tested on a device yet.**

Extra requirements: Android **NDK 27.2.12479018** (in `%LOCALAPPDATA%\Android\Sdk\ndk\`), CMake 3.18+ and
Ninja on `PATH`, and step 1 above done (the export check compares against `decoded/`).

```
cd offline\native64
python tools\fetch_deps.py         # downloads cocos2d-x 2.2.6 and libpng 1.6.44, applies the one engine patch
cmake -G Ninja -B build ^
  -DCMAKE_TOOLCHAIN_FILE=%LOCALAPPDATA%\Android\Sdk\ndk\27.2.12479018\build\cmake\android.toolchain.cmake ^
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2           # -> build\libotome.so
python tools\check_exports.py      # should print "44/44 original JNI exports present"
cd ..
python export_data.py
python export_static.py
python build_apk.py --arm64        # -> MySweetRoomies_offline.apk with lib/arm64-v8a/libotome.so added
```

`build_apk.py --arm64` packs a stripped copy of the library (about 4 MB). The unstripped one stays in
`native64\build\` so a crash log can be turned into source lines with `ndk-stack`. Build with the project in a
path without spaces if you can; CMake handles spaces, but ndk-build (not used here) doesn't. How the rewrite was
done, what differs from the original and what's still open: [offline/native64/REWRITE.md](offline/native64/REWRITE.md).

## What isn't in this repository

Some files are left out because they belong to the game's rights holders, others because they're too big or are
generated by the build (`.gitignore` has the full list).

| Left out | Size | Why | If you build without it |
|---|---|---|---|
| The original APK and `offline/decoded/` | 24 MB / 93 MB | the original game | needed: bring your own 1.4.6 APK (step 1) |
| Any built APK (`offline/*.apk`) | 1 GB | contains the original game | build it yourself |
| Story pictures, `server/data/static/img/resource/` | 7.3 GB | 9,880 frames from playthrough recordings; preserved separately and not linked from here | stories play on a plain background; a few tests fail. Put them back in that folder and rebuild |
| The official soundtrack, MP3s and the converted `server/data/static/sound/bgm/ost/` | 26 MB | copyrighted music | stories have no music, the Soundtrack menu's tracks don't play; the title music (from the APK) still does |
| `research/` (videos, OCR output, frame caches) | tens of GB | raw material for recovering the story text | not needed to build |
| `offline/tools/` (Apktool) | 24 MB | third-party tool | download Apktool 2.10.0 |
| `offline/build/`, `offline/native64/build/` | | build output | made by the build |
| `offline/offline.keystore` | | the signing key | a new one is made on first build |
| `offline/native64/cocos2d-x-cocos2d-x-2.2.6/`, `libpng-1.6.44/` | 107 MB | third-party sources | `tools/fetch_deps.py` downloads them |
| `offline/native64/re/` (Ghidra decompile of the original engine) | 4 MB | derived from the original game | not needed to build; only for further rewrite work |
| `server/data/mock.sqlite3`, logs | | local player state | made on first run |

## Documentation

[DOCUMENTATION.md](DOCUMENTATION.md) records everything: the client protocol, the crypto, how each part of the
game was recovered and from which source, the story-text pipeline, and what is confirmed versus guessed.

## Legal

This is a non-commercial preservation project, not affiliated with or endorsed by Fuji Television, Fuji・Nextera
Lab or Mobile Factory.

- **The code and documentation written for this project** are licensed under the
  [GNU Affero General Public License v3.0](LICENSE). If you run a modified version of the server for others, you
  must publish your source.
- **The game and its content** — story text, characters, art, music, voices, names and logos — belong to their
  respective owners and are **not** covered by this license. They are included only so the game can be played
  again after its shutdown. If you are a rights holder and want something removed, please open an issue.
- The original app is not included in this repository.
