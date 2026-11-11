"""Offline APK / server, soundtrack step: transcode the official soundtrack to the engine's BGM format.

The "My Sweet Roomies! (Original Soundtrack)" download is 21 MP3s; the game engine (and the title BGM it ships,
sound/bgm/13_01.ogg) uses Vorbis .ogg, so each track is re-encoded to .ogg (libvorbis -q:a 6) under the server's
static tree at static/sound/bgm/ost/<slug>.ogg, and data/soundtrack.json is written with the catalog the server
and the in-app music room read (app.py SOUNDTRACK). Resumable (skips tracks already converted). Needs ffmpeg.
usage: python convert_ost.py   (point SRC at the soundtrack folder)"""
import json
import os
import subprocess
import sys

R = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(R, "..", "My Sweet Roomies! (Original Soundtrack)_SpotifyDown_com")
DST = os.path.join(R, "..", "server", "data", "static", "sound", "bgm", "ost")
CATALOG = os.path.join(R, "..", "server", "data", "soundtrack.json")
SUFFIX = " - My Sweet Roomies!.mp3"

# name as it appears in the file -> (slug, display title, category, heroine_id or None)
# heroine ids: 01 Ren, 02 Aki, 03 Mitsuka, 04 Aoi, 05 Sayaka, 06 Misaki
META = {
    "My Sweet Roomies!": ("my_sweet_roomies", "My Sweet Roomies!", "main", None),
    "Ren Theme1":     ("ren_theme1",     "Ren – Theme 1",     "character", "01"),
    "Ren Theme2":     ("ren_theme2",     "Ren – Theme 2",     "character", "01"),
    "Aki Theme1":     ("aki_theme1",     "Aki – Theme 1",     "character", "02"),
    "Aki Theme2":     ("aki_theme2",     "Aki – Theme 2",     "character", "02"),
    "Aki Theme3":     ("aki_theme3",     "Aki – Theme 3",     "character", "02"),
    "Mitsuka Theme1": ("mitsuka_theme1", "Mitsuka – Theme 1", "character", "03"),
    "Mitsuka Theme2": ("mitsuka_theme2", "Mitsuka – Theme 2", "character", "03"),
    "Aoi Theme1":     ("aoi_theme1",     "Aoi – Theme 1",     "character", "04"),
    "Aoi Theme2":     ("aoi_theme2",     "Aoi – Theme 2",     "character", "04"),
    "Sayaka Theme1":  ("sayaka_theme1",  "Sayaka – Theme 1",  "character", "05"),
    "Sayaka Theme2":  ("sayaka_theme2",  "Sayaka – Theme 2",  "character", "05"),
    "Misaki Theme1":  ("misaki_theme1",  "Misaki – Theme 1",  "character", "06"),
    "Misaki Theme2":  ("misaki_theme2",  "Misaki – Theme 2",  "character", "06"),
    "Common":         ("common",         "Common",                 "mood", None),
    "Date":           ("date",           "Date",                   "mood", None),
    "Recollection":   ("recollection",   "Recollection",           "mood", None),
    "Noisy":          ("noisy",          "Noisy",                  "mood", None),
    "Anger":          ("anger",          "Anger",                  "mood", None),
    "Painful":        ("painful",        "Painful",                "mood", None),
    "Sadness":        ("sadness",        "Sadness",                "mood", None),
}
# Catalog order: main, then character themes in heroine order, then mood tracks.
ORDER = ["My Sweet Roomies!",
         "Ren Theme1", "Ren Theme2", "Aki Theme1", "Aki Theme2", "Aki Theme3",
         "Mitsuka Theme1", "Mitsuka Theme2", "Aoi Theme1", "Aoi Theme2",
         "Sayaka Theme1", "Sayaka Theme2", "Misaki Theme1", "Misaki Theme2",
         "Common", "Date", "Recollection", "Noisy", "Anger", "Painful", "Sadness"]


def main():
    os.makedirs(DST, exist_ok=True)
    present = {f[:-len(SUFFIX)] for f in os.listdir(SRC) if f.endswith(SUFFIX)}
    missing = present ^ set(META)
    if missing:
        print("WARNING, name mismatch vs META:", sorted(missing))
    catalog = []
    for name in ORDER:
        slug, title, cat, hid = META[name]
        src = os.path.join(SRC, name + SUFFIX)
        out = os.path.join(DST, slug + ".ogg")
        if not os.path.exists(out):
            # 44.1 kHz stereo, matching the one BGM the engine is known to decode (sound/bgm/13_01.ogg). The
            # engine's native audio is fixed at 44100 Hz; a 48 kHz source (the MP3s' rate) crashes story playback.
            subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", src, "-ar", "44100", "-ac", "2",
                            "-c:a", "libvorbis", "-q:a", "5", "-vn", out], check=True)
        entry = {"slug": slug, "title": title, "category": cat,
                 "file": f"sound/bgm/ost/{slug}.ogg"}
        if hid:
            entry["heroine_id"] = hid
        catalog.append(entry)
        print(f"  {slug:16} {os.path.getsize(out) // 1024:5} KB")
    with open(CATALOG, "w", encoding="utf8") as f:
        json.dump(catalog, f, ensure_ascii=False, indent=1)
    print(len(catalog), "tracks ->", os.path.relpath(CATALOG, R))


if __name__ == "__main__":
    sys.exit(main())
