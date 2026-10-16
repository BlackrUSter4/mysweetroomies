# My Sweet Roomies! — revival server

A reconstruction of the server for **My Sweet Roomies!** (`jp.mfapps.novel.roomies`, v1.4.6), the English
edition of Fuji TV's 「ようこそ！ファミーユへ」 (*Welcome to Famille!*). The game ran from 2014 until its servers
were shut down on 13 November 2019. The app itself still works, but it gets everything — the home screen, menus
and every story — from the server, so it has been unplayable since.

This project rebuilds that server from the decompiled app, archived pages and playthrough videos, and makes the
whole game playable again: on your PC with the original app, or fully offline with a single APK.

## Download

**[Releases](../../releases)** has `MySweetRoomies_offline.apk` (about 955 MB): the game with this server built
in. No PC, no internet, no root. It installs as **"Roomies! Offline"** next to the original app if you have it.

On first launch Android shows an "built for an older version of Android" notice and a permission screen, as it
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

**The story pictures are not in this repository** (7.3 GB). Without them the stories play on a plain background,
and a few tests that check story pictures fail. They are inside the offline APK.

## Building the offline APK

`offline/` holds the in-app server (`java/`, a port of `server/app.py` that runs inside the app) and the build
scripts. You need the original v1.4.6 APK, the story pictures, Python, a JDK and the Android SDK build tools.

```
cd offline
python export_data.py      # story list, transcripts and tables, exported from server/app.py
python export_static.py    # pictures to WebP (7.6 GB -> about 1 GB)
python build_apk.py        # patch, add the server, align and sign
```

`build_apk.py` also needs `tools/apktool.jar` (Apktool 2.10.0) and `decoded/` (the original APK decoded with
Apktool). It creates its own signing key on first run, so your build can't update the released APK in place;
uninstall that first.

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
