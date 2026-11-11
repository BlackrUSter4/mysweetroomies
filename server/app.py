"""Educational mock server for "My Sweet Roomies!" (jp.mfapps.novel.roomies, v1.4.6).

My Sweet Roomies! is the English edition of Fuji TV's 「ようこそ！ファミーユへ」
(jp.mfapps.novel.famille), Jul 2014 - Nov 13 2019. The app is a hybrid: a
Cocos2d-x shell (title screen + story engine in libotome.so) plus a WebView
that loads the game UI from the server. Production host was
http://android.roomies.novel.mfapps.jp (plain HTTP).

Reconstructed from the decompiled APK plus archived sources (official site,
store listings, screenshots). See README.md for what is confirmed vs. guessed.

Run:  python app.py [--port 80] [--apk path/to/game.apk]
"""

import argparse
import datetime
import json
import logging
import os
import random
import re
import sqlite3
import time
import uuid as uuidlib
from pathlib import Path

from flask import Flask, Response, abort, g, jsonify, render_template, request, send_from_directory
from itsdangerous import BadSignature, URLSafeSerializer

import yoga_crypto

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
STATIC_DIR = DATA_DIR / "static"
SCENARIO_DIR = DATA_DIR / "scenarios"
DB_PATH = DATA_DIR / "mock.sqlite3"

API_VERSION = 5          # JsObject.getApiVersion() returns 5
STATIC_VERSION = 1       # must be non-zero or ResourceLoaderRequest is "invalid"
FIRST_STORY_ID = "530000000001"  # app.json -> common.first_story_id (Famille used 130000000001)
STORY_PREFIX = "53"

# "Energy" (チケット in the JP edition). Confirmed from store listings and the
# 2019 shop-screen recordings: reading a scenario part costs 1 energy, max 8,
# 1 regenerates every 4 hours (the Delux monthly course cuts that to 3 hours).
TICKET_MAX = 5           # "How to Play" after the Prologue: "0/5 -> 1/5" (Eb07iH2w5L8 35:52)
SPECIAL_TICKET_MAX = 8   # Special and Delux courses: "maximizes the energy bar up to 8"
TICKET_REGEN_SECONDS = 4 * 60 * 60
DELUX_REGEN_SECONDS = 3 * 60 * 60
# Shop > Recover Energy: "Restore one energy bar with 1 crystal."
CRYSTALS_PER_ENERGY = 1
# Shop > Exchange items (Item Shop): Gold Ticket 5 crystals, Panel Ticket DX 3 crystals.
# Item Shop (Ike's Yuri Stash FNCwWtZM420 3:48): Gold Ticket 5, Panel Ticket DX 3, and the
# Memory Pass for 30/90/180 days at 9/23/40 crystals (opens every memory, like the Standard Course).
ITEM_PRICES = {"gold_ticket": 5, "panel_ticket_dx": 3, "memory_pass_30": 9, "memory_pass_90": 23, "memory_pass_180": 40}
PASS_DAYS = {"memory_pass_30": 30, "memory_pass_90": 90, "memory_pass_180": 180}
# Item screen: Gold = refill all energy, Silver = +1 energy, 4 Bronze = +1 energy.
BRONZE_PER_ENERGY = 4
GACHA_COST = 5                   # Famille's own gacha screen (神崎LEN L59 53:37): banner 「恋結晶5個」, and
                                 # 「恋結晶が1個足りません…」 with 4 owned; 恋Q部 (same engine) also shows 10 for 50.
SECRET_COST = 5                  # English popup "Get the Secret Story!!" ... "You need N crystal(s) more!" / "The
                                 # total amount of Crystal : 5 -> 0" + "Get it NOW!" (SandDistress swmPbeU6CPQ 5:33;
                                 # Chicoine p99LNijyFWc 12:58); JP 裏話 also 5 恋結晶 (famille.gamerch.com)
EPILOGUE_COST = 10               # JP: Misaki's epilogue needed 10 恋結晶 (神崎LEN tJTshlUQW5E 5:20); English
                                 # players also paid for epilogues (PokeCommunity 398416, 2018-01-06)
# Login streak raises the energy cap (English tutorial, PoISoN2462 EqwSQBUXVfE 12:14):
# 5 on day 1, 6 after 2 straight days, 7 after 5, 8 after 8; "If you forget to login, your max
# energy will go down." The predecessor ただいまっ!うちカノジョ's identical page says how far:
# 「ログインをサボると…チケット上限が5枚にダウン!」 (inoue tamaki mtlCHtI8VmM 0:22), so a missed day
# restarts the streak.
STREAK_CAPS = [(8, 8), (5, 7), (2, 6), (1, 5)]
NEW_PLAYER_CRYSTALS = 10         # JP pre-registration bonus: 10 恋結晶 ("worth ¥1,000")
NEW_PLAYER_GOLD_TICKETS = 1
PANELS_PER_PICTURE = 16          # daily login removes one panel of a hidden picture: a 4x4 grid,
                                 # one picture per ~15-day period (JP 神崎LEN kC7sdrz4JYw 1:12)
DEFAULT_PLAYER_NAME = "William"  # editable; players' videos show custom names (e.g. "Mihile")
# Monthly Course Shop (subscription SKUs subscription.bronze/silver/gold):
COURSES = {
    "standard": {"sku": "jp.mfapps.novel.roomies.item.subscription.bronze", "price": "$2.78",
                 "text": "Standard Course provides that all memories you have got open up."},
    "special": {"sku": "jp.mfapps.novel.roomies.item.subscription.silver",
                "text": "In addition to the Standard Course, Special Course maximizes the energy bar up to 8."},
    "delux": {"sku": "jp.mfapps.novel.roomies.item.subscription.gold",
              "text": "In addition to the Standard and Special Courses, 4 hours of energy restoration time is cut to 3 hours!"},
}

app = Flask(__name__)
app.config["SECRET_KEY"] = os.environ.get("MOCK_SECRET", "dev-only-not-secret")
app.config["AES_KEY"] = bytes.fromhex(yoga_crypto.ORIGINAL_KEY_HEX)
app.config["MAINTENANCE"] = False

log = logging.getLogger("roomies")
session_signer = URLSafeSerializer(app.config["SECRET_KEY"], salt="mock-session")


# --------------------------------------------------------------------------- #
# Static game data
# --------------------------------------------------------------------------- #

def load_fixture(path):
    """Real captured JSON wins over generated defaults. Returns None if absent."""
    if path.is_file():
        with path.open(encoding="utf-8") as f:
            return json.load(f)
    return None


HEROINES = load_fixture(DATA_DIR / "heroines.json")
HEROINE_BY_ID = {h["id"]: h for h in HEROINES}

# The official soundtrack (My Sweet Roomies! Original Soundtrack, 21 tracks), transcoded to the engine's BGM
# format (Vorbis .ogg, like sound/bgm/13_01.ogg) under static/sound/bgm/ost/ (offline/convert_ost.py). Each
# entry: slug, display title, category (main|character|mood), file path, and heroine_id for a character theme.
SOUNDTRACK = load_fixture(DATA_DIR / "soundtrack.json") or []
SOUNDTRACK_BY_SLUG = {t["slug"]: t for t in SOUNDTRACK}
# A story plays its heroine's first character theme; the shared prologue and any heroine-less story use the main
# theme. The original per-scene tracks were downloads from the live server and are lost, so this is a best-effort
# match rather than the exact in-game cue (see engine_format).
HEROINE_BGM = {t["heroine_id"]: t["file"] for t in SOUNDTRACK
               if t.get("category") == "character" and t["title"].endswith("Theme 1")}
MAIN_BGM = next((t["file"] for t in SOUNDTRACK if t.get("category") == "main"), "sound/bgm/13_01.ogg")


# Route structure from data/chapter_titles.json: an ordered list of chapters per
# heroine with the label and title printed on each in-game title card. Lengths
# were counted from a 2015-2017 YouTube upload of every chapter (Ren 53, Aki 57,
# Mitsuka 52, Aoi 55 + a duplicate "34", Sayaka 52, Misaki 54) + 2 epilogues each.
CHAPTER_TITLES = load_fixture(DATA_DIR / "chapter_titles.json") or {}
FALLBACK_CHAPTERS = {"01": 53, "02": 57, "03": 52, "04": 56, "05": 52, "06": 54}
# Chapters are split into parts that each load as a separate scenario
# (e.g. "ch.20 uproar 1-5"). data/chapter_parts.json holds per-chapter counts
# measured from playthrough videos (loading screens between parts); chapters
# without a measurement use DEFAULT_PARTS.
CHAPTER_PARTS = load_fixture(DATA_DIR / "chapter_parts.json") or {}
DEFAULT_PARTS = 3


def parts_for(heroine_id, chapter_index):
    counts = CHAPTER_PARTS.get(heroine_id) or []
    n = counts[chapter_index - 1] if chapter_index <= len(counts) else None
    return n or DEFAULT_PARTS


def chapters_for(heroine_id):
    return CHAPTER_TITLES.get(heroine_id) or [
        {"label": str(i), "title": None} for i in range(1, FALLBACK_CHAPTERS[heroine_id] + 1)]


def story_id(heroine_id, chapter, part=1, arc="01"):
    """12-digit scenario id. MainStoryJson reads the partner from [2:4] and the
    chapter from [8:10]. The Prologue is 530000000001/02 (app.json first_story_id
    plus the next part), so [10:12] is read as the part. arc "02" for epilogues
    is a guess."""
    return f"{STORY_PREFIX}{heroine_id}{arc}01{chapter:02d}{part:02d}"


def default_stories():
    # Memory screen tabs: Main Story / Secret Story / Epilogue / Event Story.
    stories = [{"scenario_id": f"{STORY_PREFIX}00000000{p:02d}", "heroine_id": "00", "kind": "prologue",
                "chapter": 0, "part": p, "title": f"Prologue {p}", "story_type": "main",
                "partner_name": "Everyone"} for p in (1, 2)]
    for h in HEROINES:
        for ch, info in enumerate(chapters_for(h["id"]), 1):
            label = f"Chapter {info['label']}"
            for p in range(1, parts_for(h["id"], ch) + 1):
                stories.append({"scenario_id": story_id(h["id"], ch, p), "heroine_id": h["id"], "kind": "main",
                                "chapter": ch, "part": p, "story_type": "main", "partner_name": h["name"],
                                "title": f"{label} - {info['title']}" if info["title"] else label})
        for ep in (1, 2):
            stories.append({"scenario_id": story_id(h["id"], ep, 1, arc="02"), "heroine_id": h["id"],
                            "kind": "epilogue", "chapter": ep, "part": 1, "story_type": "epilogue",
                            "partner_name": h["name"], "title": f"Epilogue {ep}"})
    # Secret stories go last so the main chain (each part unlocks the next) stays intact.
    by_id = {h["id"]: h for h in HEROINES}
    for sec in (load_fixture(DATA_DIR / "secret_stories.json") or {}).get("stories", []):
        h = by_id.get(sec["heroine_id"])
        if not h:
            continue
        info = chapters_for(h["id"])[sec["chapter"] - 1]
        stories.append({"scenario_id": story_id(h["id"], sec["chapter"], sec["number"], arc="03"),
                        "heroine_id": h["id"], "kind": "secret", "chapter": sec["chapter"], "part": sec["number"],
                        "story_type": "secret", "partner_name": h["name"], "title": sec["title"],
                        "after_part": sec.get("after_part"),
                        "chapter_title": f"Chapter {info['label']} - {info['title']}" if info["title"] else f"Chapter {info['label']}"})
    return stories


STORIES = load_fixture(DATA_DIR / "stories.json") or default_stories()
STORY_BY_ID = {s["scenario_id"]: s for s in STORIES}
STORY_INDEX = {s["scenario_id"]: i for i, s in enumerate(STORIES)}


# Each route has two halves (Japanese 神崎LEN playthroughs: a 「クリア!!」 popup with 「エピローグ出現!!」
# after the first half, "まさかこれがエンディングなんて思ってるんじゃない？最後に取っておきのストーリーが
# 残されてるわよ", the progress meter restarts at 0%, then a second clear at the route's end).
# Epilogue 1 opens after the first half, Epilogue 2 after the last chapter; the main story goes on
# without them (the popup has 「今はやめておこう」). Last chapter label of the first half (first
# occurrence: Aoi's first "34" is JP 33 「…を乗り越えて」 = "Overcoming It All").
FIRST_HALF_LAST = {"01": "31", "02": "34", "03": "32", "04": "34", "05": "32", "06": "30"}


def first_half_last_chapter(heroine_id):
    for i, c in enumerate(chapters_for(heroine_id), 1):
        if c["label"] == FIRST_HALF_LAST.get(heroine_id):
            return i
    return None


def previous_story(story):
    """Scenarios unlock in order: Prologue 1 -> 2 -> a route's parts; epilogues and secret stories
    branch off the main chain at their unlock point."""
    i = STORY_INDEX[story["scenario_id"]]
    if story["kind"] == "secret":
        # Listed under its chapter once the part that unlocks it is read (p99LNijyFWc 19:29; the
        # unlocking part is known from the Japanese 「裏話出現!!」 popups); else the chapter's last part.
        last = [s for s in STORIES if s["heroine_id"] == story["heroine_id"] and s["kind"] == "main"
                and s["chapter"] == story["chapter"]]
        if story.get("after_part"):
            last = [s for s in last if s["part"] <= story["after_part"]] or last
        return last[-1]["scenario_id"] if last else None
    if story["kind"] == "prologue":
        return STORIES[i - 1]["scenario_id"] if story["part"] > 1 else None
    if story["kind"] == "epilogue":
        mains = [s for s in STORIES if s["heroine_id"] == story["heroine_id"] and s["kind"] == "main"]
        half = first_half_last_chapter(story["heroine_id"])
        if story["chapter"] == 1 and half:
            mains = [s for s in mains if s["chapter"] <= half]
        return mains[-1]["scenario_id"] if mains else None
    if i > 0 and STORIES[i - 1]["heroine_id"] == story["heroine_id"]:
        return STORIES[i - 1]["scenario_id"]
    return f"{STORY_PREFIX}0000000002"  # first chapter of any route needs the Prologue


# Dress-up outfits from the gacha. Every heroine starts in her usual clothes ("Casual").
# The English edition's four with art: DevilGh0stz's 1080p home-screen screenshots
# (imgur 7CWVK, 2017-11-23), cut out by research/costume_cutout.py into
# img/heroine/costume/<heroine>_<file>.png. The item names in the game weren't recorded, so
# these are descriptive. The Japanese tutorial also shows Misaki in a red coat, Mitsuka in a
# white dress, Aoi in a light dress, Ren in cat ears and Sayaka as a nurse (no art recovered).
DEFAULT_COSTUME = "Casual"
# Ren's six (2026-10-12) come from the "Mega Assets" fan-made pack
# (research/mega_assets/): a Traditional Chinese edition home screenshot (blouse) and the official
# Japanese event cards ファミーユカフェ (maid), スクールフェス (idol), カノジョとふたりですごす夏
# (yukata), 海辺のシンデレラ (swimsuit) and 彼女がドレスに着替えたら (wedding), cut out by
# research/ren_outfits.py. Names are descriptive.
# The other heroines' cards in the pack (2026-10-13, research/mega_outfits.py) add the same five
# events plus ファミーユ☆ポリス (police). Sayaka's maid card isn't used: the English screenshot is better.
# Taiwan-edition wardrobe home shots (research/tw_wardrobe.py, picked/named in research/tw_costumes.py) are the
# "tw<nn>" files; they replaced the Mega Assets card cut-outs of the same outfits. The rest: card cut-outs
# (mega_outfits.py) and Chinese-edition home shots (aki_home_outfits.py).
COSTUMES = {"01": {"White Blouse": "tw24", "Maid Cafe": "maid", "School Idol": "tw25", "Summer Yukata": "yukata",
                   "Swimsuit": "swimsuit", "Wedding Dress": "wedding", "Cat-Ear Maid": "catmaid",
                   "Blue Frill Top": "tw04", "Sky-Blue Shirt": "tw06", "White Yukata": "tw07",
                   "White Shirt": "tw08", "Trench Coat": "tw10", "White Hoodie": "tw11", "Pearl Blouse": "tw12",
                   "Grey Coat": "tw13", "Lace Blouse": "tw14", "Open Shirt": "tw15", "Haori": "tw17",
                   "Santa Dress": "tw18", "New Year Kimono": "tw19", "School Blazer": "tw20", "Wrap Top": "tw21",
                   "Police Uniform": "tw22", "Houndstooth Vest": "tw23", "Backpack Outfit": "tw26",
                   "Farm Apron": "tw27"},
            "02": {"Winter Coat": "coat", "Maid Cafe": "tw15", "School Idol": "tw24", "Police Uniform": "tw21",
                   "Swimsuit": "swimsuit", "Wedding Dress": "wedding", "Mint Camisole": "camisole",
                   "Cardigan Dress": "cardigan", "Bathrobe": "bathrobe", "Coat and Scarf": "scarf",
                   "Santa Dress": "santa", "Fur Parka": "parka", "Frilly Blouse": "frills", "Pink Hoodie": "hoodie",
                   "Witch": "witch", "Gothic Dress": "gothic", "Navy Yukata": "yukata2",
                   "Pink Ruffle Blouse": "tw07", "Off-Shoulder Top": "tw09", "Haori": "tw16",
                   "Ascot Blouse": "tw17", "New Year Kimono": "tw19", "School Blazer": "tw20",
                   "Houndstooth Vest": "tw22", "Gingham Polo": "tw25"},
            "03": {"Maid Cafe": "tw16", "School Idol": "tw25", "Police Uniform": "tw22", "Summer Yukata": "tw08",
                   "Wedding Dress": "tw28", "Striped Camisole": "tw02", "Overalls": "tw06",
                   "Pink Ruffle Top": "tw07", "Argyle Vest": "tw09", "Down Vest": "tw10", "Pink Cardigan": "tw11",
                   "Coral Sweater": "tw12", "Duffle Coat": "tw13", "Blue-Bow Maid": "tw14", "Haori": "tw17",
                   "Santa Capelet": "tw18", "New Year Kimono": "tw19", "School Blazer": "tw20", "Bathrobe": "tw21",
                   "Houndstooth Vest": "tw23", "Green Cardigan": "tw24", "White Shirt": "tw26",
                   "Tuxedo Vest": "tw27"},
            "04": {"Swimsuit": "tw05", "School Idol": "tw26", "Summer Yukata": "tw08", "Wedding Dress": "tw27",
                   "White Camisole": "tw07", "Track Jacket": "tw09", "Denim Jacket": "tw10", "Raglan Tee": "tw11",
                   "Grey Suit": "tw12", "Blue Jacket": "tw13", "Military Coat": "tw14", "Tank Top": "tw15",
                   "Gothic Dress": "tw16", "Haori": "tw17", "Ascot Blouse": "tw18", "Santa Dress": "tw19",
                   "New Year Kimono": "tw20", "School Blazer": "tw21", "Bathrobe": "tw22", "Police Uniform": "tw23",
                   "Black Suit": "tw24", "Pearl Blouse": "tw25",
                   "Sequin Halter": "tw28"},
            "05": {"Haori": "tw20", "Maid Outfit": "tw19", "Police Uniform": "tw26", "Summer Yukata": "tw18",
                   "Swimsuit": "tw06", "Purple Camisole": "tw08", "Lilac Wrap Top": "tw09", "White Tee": "tw10",
                   "Army Jacket": "tw11", "Rose Dress": "tw12", "Pink Halter": "tw13", "Red Cardigan": "tw14",
                   "Fur Collar Coat": "tw15", "White Shirt": "tw16", "Nurse Uniform": "tw17",
                   "Ascot Blouse": "tw21", "Santa Dress": "tw22", "New Year Kimono": "tw23",
                   "School Blazer": "tw24", "Bathrobe": "tw25", "Office Suit": "tw27", "Knit Sweater": "tw28",
                   "Ribbon Blouse": "tw29", "Orange Kimono": "tw30", "Blue Halter": "tw31"},
            "06": {"Maid Cafe": "tw16", "Summer Yukata": "tw08", "Wedding Dress": "wedding", "Black Halter": "tw05",
                   "Red Shirt": "tw07", "Off-Shoulder Knit": "tw09", "Khaki Cardigan": "tw10",
                   "Brown Vest Top": "tw11", "Salmon Shirt": "tw12", "Denim Jacket": "tw13", "Red Parka": "tw14",
                   "Ruffle Shirt": "tw15", "Haori": "tw17", "Ascot Blouse": "tw18", "Santa Dress": "tw19",
                   "Purple Kimono": "tw20", "Grey Suit": "tw21", "Bathrobe": "tw22", "Police Uniform": "tw23",
                   "Blue Shirt": "tw24", "Black Jacket": "tw25", "White Blouse": "tw26", "Evening Dress": "tw27"}}
# Card art (2026-10-15): the Mega Assets card cut-outs that the TW home shots replaced show more of her (waist or
# full length against the TW shots' mid-chest bust), so they're offered again as "<outfit> (Card)" beside the TW
# version. They aren't gacha prizes: owning the outfit unlocks its card version. No face layer (other head art).
CARD_SUFFIX = " (Card)"
CARD_ART = {"01": {"School Idol": "idol"},
            "02": {"Maid Cafe": "maid", "School Idol": "idol", "Police Uniform": "police"},
            "03": {"Maid Cafe": "maid", "School Idol": "idol", "Police Uniform": "police", "Summer Yukata": "yukata",
                   "Wedding Dress": "wedding"},
            "04": {"Swimsuit": "swimsuit", "School Idol": "idol", "Summer Yukata": "yukata", "Wedding Dress": "wedding"},
            "05": {"Maid Outfit": "maid", "Police Uniform": "police", "Swimsuit": "swimsuit", "Haori": "yukata",
                   "Summer Yukata": "yukata2"},
            "06": {"Maid Cafe": "maid", "Summer Yukata": "yukata"}}
# Every wearable outfit -> file, each card version right after its outfit (gacha draws from COSTUMES only).
ALL_COSTUMES = {hid: {k: v for name, f in pool.items()
                      for k, v in [(name, f)] + ([(name + CARD_SUFFIX, CARD_ART[hid][name])]
                                                 if name in CARD_ART.get(hid, {}) else [])}
                for hid, pool in COSTUMES.items()}


def card_base(heroine_id, costume):
    """The outfit a card version belongs to, or None."""
    if costume and costume.endswith(CARD_SUFFIX):
        base = costume[:-len(CARD_SUFFIX)]
        if base in CARD_ART.get(heroine_id, {}):
            return base
    return None


# Dress Up > Background: the items seen on a Chinese store ad of the Dress Up screen (Mega Assets pack):
# the house (the home screen's own), a vegetable garden, a bridge at sunset, cherry blossoms, a beach, then "?".
# research/dress_backgrounds.py recovered the garden and the beach from story pictures; the bridge and the
# cherry blossoms weren't found (shown locked). How backgrounds were obtained isn't known, so the mock has them
# all available; the choice is kept per heroine and drawn behind her on the home screen.
# The Taiwan board (snA=182 #1) lists 13: outside the house morning/evening, each heroine's room, the living room,
# the balcony morning/evening/night and the school cafeteria. research/location_bgs.py found 8 of them as empty story
# pictures (the morning exterior is the house itself); Mitsuka's and Misaki's rooms were added 2026-10-15. Sayaka's
# room never appears in the story and the balcony only at evening and night, so those two stay locked. Both lists are kept: the ad's items aren't on the board's list.
DEFAULT_BG = "Famille House"
BACKGROUNDS = {DEFAULT_BG: "img/heroine/home_bg.png", "Vegetable Garden": "img/heroine/bg/garden.png",
               "Sunset Bridge": None, "Cherry Blossoms": None, "Beach": "img/heroine/bg/beach.png",
               "Famille House (Evening)": "img/heroine/bg/outside_evening.png",
               "Living Room": "img/heroine/bg/living_room.png", "Ren's Room": "img/heroine/bg/room_ren.png",
               "Aki's Room": "img/heroine/bg/room_aki.png", "Mitsuka's Room": "img/heroine/bg/room_mitsuka.png",
               "Aoi's Room": "img/heroine/bg/room_aoi.png", "Sayaka's Room": None, "Misaki's Room": "img/heroine/bg/room_misaki.png",
               "Balcony (Morning)": None, "Balcony (Evening)": "img/heroine/bg/balcony_evening.png",
               "Balcony (Night)": "img/heroine/bg/balcony_night.png", "School Cafeteria": "img/heroine/bg/cafeteria.png"}

# Dress Up > Face: the Taiwan board names 9 expressions (snA=182 #1). research/face_items.py lifted a face for each
# from the story pictures, whose sprites share the frontal head of the TW wardrobe outfits, so a face can be laid
# over those outfits only (data/face_layout.json: where each one's head sits). "Default" is the outfit's own face.
DEFAULT_FACE = "Default"
_FACES = load_fixture(DATA_DIR / "faces.json") or {}
FACE_NAMES = [n["en"] for n in _FACES.get("names", [])]
FACES = {k: v for k, v in _FACES.items() if not k.startswith("_") and k != "names"}       # id -> {box, faces}
FACE_LAYOUT = load_fixture(DATA_DIR / "face_layout.json") or {}


def face_overlay(heroine_id, costume, face):
    """The chosen face over her outfit, in the outfit picture's pixels, for the home screen to draw on top."""
    f = ALL_COSTUMES.get(heroine_id, {}).get(costume)
    lay, hf = FACE_LAYOUT.get(f"{heroine_id}_{f}"), FACES.get(heroine_id, {})
    art = hf.get("faces", {}).get(face)
    if not (lay and art and face != DEFAULT_FACE):
        return None
    bx, by, bw, bh = hf["box"]
    return {"url": f"/static/{STATIC_VERSION}/img/heroine/face/{art}", "x": lay["dx"] + bx, "y": lay["dy"] + by,
            "w": bw, "h": bh, "iw": lay["w"], "ih": lay["h"]}


# Touching the heroine on the home screen plays a short scene in the story viewer, then an
# "EXP x%" popup with her line (research/touch_scenes.py; Aki, Sayaka and Aoi recorded).
# The scenario id's arc "09" is the mock's own; the real one wasn't captured.
TOUCH_SCENES = load_fixture(DATA_DIR / "touch_scenes.json") or {}
# Touching her on the home screen shows a chat bubble (data/touch_lines.json, JP lines read from
# 神崎LEN's playthrough, English translated); recorded touch scenes still play now and then.
TOUCH_LINES = {k: v for k, v in (load_fixture(DATA_DIR / "touch_lines.json") or {}).items() if not k.startswith("_")}
TOUCH_BY_ID = {f"{STORY_PREFIX}{hid}09010101": hid for hid in TOUCH_SCENES}
TOUCH_ID = {hid: sid for sid, hid in TOUCH_BY_ID.items()}
NEWS = load_fixture(DATA_DIR / "news.json") or []

# MENU > Photo has three tabs: Photo Panel, Event, Epilogue (DBzQdN-tyn4 3:57:20). A panel
# picture is the prize of one ~2-week login-panel period; each has a caption "Aoi : Wake-Up
# Call" and its period. Epilogue pictures are kept per epilogue part. research/photo_pictures.py
# recovered Aoi's four panel pictures and her Epilogue 1 picture; the mock hands out the panel
# pictures in order as pictures are completed.
PHOTOS = (load_fixture(DATA_DIR / "photos.json") or {}).get("pictures", [])
# Event Stories: names (and a few banners); text survives for the Edo event's preview and Sayaka's episode 1 and for
# Sayaka's and Aoi's routes of the hot-spring event, from Taiwan recordings (data/event_stories.json "parts";
# research/tw_secret_stories.py, tw_edo_gacha.py, tw_hotspring.py).
EVENTS = (load_fixture(DATA_DIR / "event_stories.json") or {}).get("events", [])
EVENT_SCENARIOS = {e["scenario_id"] for e in EVENTS if e.get("scenario_id")} | {
    p["scenario_id"] for e in EVENTS for p in e.get("parts", [])}
# A heroine's event episode ends on "To Be Continued" (HFtNCIJ2i1Q), except the last of her route ("final"); the
# preview is a one-off.
EVENT_EPISODES = {p["scenario_id"] for e in EVENTS for p in e.get("parts", []) if p.get("heroine_id") and not p.get("final")}
PANEL_PHOTOS = [p for p in PHOTOS if p["kind"] == "panel"]


# --------------------------------------------------------------------------- #
# Storage
# --------------------------------------------------------------------------- #

def db():
    if "db" not in g:
        g.db = sqlite3.connect(DB_PATH)
        g.db.row_factory = sqlite3.Row
    return g.db


@app.teardown_appcontext
def close_db(_exc):
    conn = g.pop("db", None)
    if conn is not None:
        conn.close()


def init_db():
    DATA_DIR.mkdir(exist_ok=True)
    with sqlite3.connect(DB_PATH) as conn:
        conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS users (
                uuid            TEXT PRIMARY KEY,
                device_id       TEXT,
                invite_id       TEXT NOT NULL,
                name            TEXT NOT NULL DEFAULT 'William',
                heroine_id      TEXT NOT NULL DEFAULT '04',
                crystals        INTEGER NOT NULL DEFAULT 0,
                gold_tickets    INTEGER NOT NULL DEFAULT 0,
                silver_tickets  INTEGER NOT NULL DEFAULT 0,
                bronze_tickets  INTEGER NOT NULL DEFAULT 0,
                panel_tickets   INTEGER NOT NULL DEFAULT 0,
                course          TEXT,
                tickets         INTEGER NOT NULL,
                tickets_at      INTEGER NOT NULL,
                panels          INTEGER NOT NULL DEFAULT 0,
                pictures_done   INTEGER NOT NULL DEFAULT 0,
                last_login_day  TEXT,
                created_at      INTEGER NOT NULL,
                last_seen       INTEGER NOT NULL
            );
            CREATE TABLE IF NOT EXISTS story_read (
                uuid        TEXT NOT NULL,
                scenario_id TEXT NOT NULL,
                read_at     INTEGER NOT NULL,
                PRIMARY KEY (uuid, scenario_id)
            );
            CREATE TABLE IF NOT EXISTS costumes (
                uuid        TEXT NOT NULL,
                heroine_id  TEXT NOT NULL,
                costume     TEXT NOT NULL,
                equipped    INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (uuid, heroine_id, costume)
            );
            CREATE TABLE IF NOT EXISTS dress_bg (
                uuid        TEXT NOT NULL,
                heroine_id  TEXT NOT NULL,
                bg          TEXT NOT NULL,
                PRIMARY KEY (uuid, heroine_id)
            );
            CREATE TABLE IF NOT EXISTS dress_face (
                uuid        TEXT NOT NULL,
                heroine_id  TEXT NOT NULL,
                face        TEXT NOT NULL,
                PRIMARY KEY (uuid, heroine_id)
            );
            CREATE TABLE IF NOT EXISTS purchases (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                uuid        TEXT,
                product_id  TEXT,
                raw         TEXT,
                created_at  INTEGER NOT NULL
            );
            """
        )
        cols = [r[1] for r in conn.execute("PRAGMA table_info(users)")]
        if "pass_until" not in cols:
            conn.execute("ALTER TABLE users ADD COLUMN pass_until INTEGER")
        if "login_days" not in cols:
            conn.execute("ALTER TABLE users ADD COLUMN login_days INTEGER NOT NULL DEFAULT 0")
        if "login_streak" not in cols:
            conn.execute("ALTER TABLE users ADD COLUMN login_streak INTEGER NOT NULL DEFAULT 0")


def now():
    return int(time.time())


def get_or_create_user(auth):
    """auth is the decrypted X-Yoga-Authorization JSON (or None)."""
    if not auth or not auth.get("uuid"):
        return None
    t = now()
    row = db().execute("SELECT * FROM users WHERE uuid = ?", (auth["uuid"],)).fetchone()
    if row is None:
        invite_id = uuidlib.uuid4().hex[:8].upper()
        db().execute(
            "INSERT INTO users (uuid, device_id, invite_id, crystals, gold_tickets, tickets,"
            " tickets_at, created_at, last_seen) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (auth["uuid"], auth.get("device_id"), invite_id, NEW_PLAYER_CRYSTALS,
             NEW_PLAYER_GOLD_TICKETS, TICKET_MAX, t, t, t),
        )
        for h in HEROINES:
            db().execute("INSERT INTO costumes VALUES (?, ?, ?, 1)", (auth["uuid"], h["id"], DEFAULT_COSTUME))
        log.info("new player %s (invite id %s)", auth["uuid"], invite_id)
    else:
        db().execute("UPDATE users SET last_seen = ? WHERE uuid = ?", (t, auth["uuid"]))
    db().commit()
    return db().execute("SELECT * FROM users WHERE uuid = ?", (auth["uuid"],)).fetchone()


def reload_user():
    g.user = db().execute("SELECT * FROM users WHERE uuid = ?", (g.user["uuid"],)).fetchone()
    return g.user


# --------------------------------------------------------------------------- #
# Tickets: stored count + timestamp, regenerated lazily on read
# --------------------------------------------------------------------------- #

def regen_seconds(user):
    course = user["course"] if "course" in user.keys() else None
    return DELUX_REGEN_SECONDS if course == "delux" else TICKET_REGEN_SECONDS


def ticket_max(user):
    course = user["course"] if "course" in user.keys() else None
    streak = user["login_streak"] if "login_streak" in user.keys() else 0
    cap = next((c for days, c in STREAK_CAPS if streak >= days), TICKET_MAX)
    return max(cap, SPECIAL_TICKET_MAX if course in ("special", "delux") else TICKET_MAX)


def ticket_state(user, t=None):
    t = t or now()
    regen, top = regen_seconds(user), ticket_max(user)
    tickets, anchor = user["tickets"], user["tickets_at"]
    if tickets >= top:   # can be above the cap after a course ends; it just stops regenerating
        return {"tickets": tickets, "max": top, "next_in": 0, "anchor": t}
    gained = (t - anchor) // regen
    tickets = min(top, tickets + gained)
    anchor = t if tickets >= top else anchor + gained * regen
    next_in = 0 if tickets >= top else regen - (t - anchor)
    return {"tickets": tickets, "max": top, "next_in": next_in, "anchor": anchor}


def add_energy(n):
    """Add n energy (capped at max), keeping the regen clock running."""
    t = ticket_state(g.user)
    total = min(t["max"], t["tickets"] + n)
    save_tickets(total, now() if total >= t["max"] else t["anchor"])


def save_tickets(tickets, anchor):
    db().execute("UPDATE users SET tickets = ?, tickets_at = ? WHERE uuid = ?",
                 (tickets, anchor, g.user["uuid"]))


# --------------------------------------------------------------------------- #
# Request inspection: decode the Yoga headers on every request
# --------------------------------------------------------------------------- #

YOGA_HEADERS = (
    "X-Yoga-Authorization", "X-Package-Name", "X-Version-Code", "X-Version-Name",
    "X-Build-Info", "X-Yoga-Request-From", "X-Yoga-Kpi-ID", "X-Push-Token",
)


@app.before_request
def inspect_request():
    g.auth = None
    g.user = None

    token = request.headers.get("X-Yoga-Authorization")
    if token:
        try:
            g.auth = json.loads(yoga_crypto.decrypt(app.config["AES_KEY"], token))
        except Exception as exc:  # wrong key (re-signed APK?) or garbage
            log.warning("could not decrypt X-Yoga-Authorization: %s", exc)

    # Sub-requests made by the WebView itself (XHR, images) do NOT carry the
    # Yoga headers, so our front-end sends a signed session token instead.
    if g.auth is None:
        sess = request.headers.get("X-Mock-Session") or request.args.get("_s")
        if sess:
            try:
                g.auth = {"uuid": session_signer.loads(sess)}
            except BadSignature:
                pass

    # Desktop-browser testing: /?dev=1 acts as a fixed fake device.
    if g.auth is None and request.args.get("dev"):
        g.auth = {"uuid": "dev-" + re.sub(r"\W", "", request.args["dev"])[:32], "device_id": "browser"}

    if g.auth:
        g.user = get_or_create_user(g.auth)

    yoga = {h: request.headers[h] for h in YOGA_HEADERS if h in request.headers}
    log.info("%s %s", request.method, request.full_path.rstrip("?"))
    if yoga.get("X-Package-Name"):
        log.info("    client: %s v%s (%s) build=%s",
                 yoga.get("X-Package-Name"), yoga.get("X-Version-Name"),
                 yoga.get("X-Version-Code"), yoga.get("X-Build-Info"))
    if g.auth:
        log.info("    auth:   %s", g.auth)


@app.after_request
def strip_json_newline(resp):
    # NetworkClient.isJsonResponse() requires the body to END with "}" or "]" and
    # matches ^\s*(\{|\[).*(\}|\])\s*$ without DOTALL. Flask's jsonify appends "\n",
    # which made the real client reject every native JSON response ("not json").
    if resp.mimetype == "application/json" and not resp.direct_passthrough:
        body = resp.get_data()
        if body.endswith(b"\n"):
            resp.set_data(body.rstrip(b"\n"))
    return resp


def require_user():
    if not g.user:
        abort(401)


@app.errorhandler(401)
def unauthorized(_e):
    # The only archived response from the real server (2017-06-02):
    # GET / without auth -> 401, body "Unauthorized".
    return Response("Unauthorized", 401, mimetype="text/plain")


# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #

def site_url():
    """The host the client used to reach us, e.g. http://android.roomies.novel.mfapps.jp"""
    return request.host_url.rstrip("/")


def proxy_json(content, **extra):
    """Envelope parsed by jp.mfapps.framework.maidengine.model.json.ProxyJson."""
    body = {
        "api_version": API_VERSION,
        "timestamp": now(),
        "version": 1,
        "cache_lifetime": 0,
        "is_maintenance": 1 if app.config["MAINTENANCE"] else 0,
        "error": None,
        "location": None,
        "content": content,
    }
    body.update(extra)
    return jsonify(body)


def valid_story_id(sid):
    if not re.fullmatch(r"[0-9A-Za-z_-]{1,32}", sid):
        abort(400, "bad story id")
    return sid


def read_ids(uuid):
    return {r["scenario_id"] for r in db().execute(
        "SELECT scenario_id FROM story_read WHERE uuid = ?", (uuid,))}


def heroine_progress(read):
    out = {}
    for h in HEROINES:
        eps = [s for s in STORIES if s["heroine_id"] == h["id"] and s["kind"] == "main"]
        done = sum(1 for s in eps if s["scenario_id"] in read)
        out[h["id"]] = round(100.0 * done / len(eps), 1) if eps else 0.0
    return out


def stand_url(heroine_id, costume=None):
    f = ALL_COSTUMES.get(heroine_id, {}).get(costume)
    rel = f"img/heroine/costume/{heroine_id}_{f}.png" if f else f"img/heroine/{heroine_id}_stand.png"
    if not (STATIC_DIR / rel).is_file():
        rel = f"img/heroine/{heroine_id}_stand.png"
    return f"/static/{STATIC_VERSION}/{rel}" if (STATIC_DIR / rel).is_file() else None


def photo_entry(p):
    h = HEROINE_BY_ID.get(p["heroine_id"], {})
    if p["kind"] != "panel":
        caption = p["title"]
    else:                   # Mega Assets pictures: some titles weren't recorded (research/cg_photos.py)
        caption = f"{h.get('short', '')} : {p['title']}" if p["title"] else h.get("short", "")
    return {"kind": p["kind"], "url": f"/static/{STATIC_VERSION}/img/photo/{p['name']}.png", "caption": caption,
            "sub": p["period"] or ("" if p["title"] or p["kind"] != "panel" else "Title not recorded")
            or h.get("short", "")}


def player_state():
    u = g.user
    read = read_ids(u["uuid"])
    tickets = ticket_state(u)
    equipped = {r["heroine_id"]: r["costume"] for r in db().execute(
        "SELECT heroine_id, costume FROM costumes WHERE uuid = ? AND equipped = 1", (u["uuid"],))}
    owned = {}
    for r in db().execute("SELECT heroine_id, costume FROM costumes WHERE uuid = ?", (u["uuid"],)):
        if r["costume"] == DEFAULT_COSTUME or r["costume"] in COSTUMES.get(r["heroine_id"], {}):
            owned.setdefault(r["heroine_id"], []).append(r["costume"])   # drops the old guessed outfits
            if r["costume"] in CARD_ART.get(r["heroine_id"], {}):          # ... and unlocks its card version
                owned[r["heroine_id"]].append(r["costume"] + CARD_SUFFIX)
    progress = heroine_progress(read)
    bgs = {r["heroine_id"]: r["bg"] for r in db().execute("SELECT heroine_id, bg FROM dress_bg WHERE uuid = ?", (u["uuid"],))
           if BACKGROUNDS.get(r["bg"])}
    faces = {r["heroine_id"]: r["face"] for r in db().execute("SELECT heroine_id, face FROM dress_face WHERE uuid = ?",
                                                             (u["uuid"],))}
    # The Character screen shows one heroine as a grey "???" card (YYlJJHsm0t0 4:03):
    # Misaki, the landlady, is a hidden route that opens once the other five are finished:
    # DevilGh0stz on 2016-01-11 (mIkmTpGmXxM), with Aoi, Ren and Sayaka already played,
    # "still got Aki to go in order to unlock Misaki".
    misaki_open = progress.get("06", 0) > 0 or all(v >= 100 for k, v in progress.items() if k != "06")
    heroines = [{**h, "progress": progress[h["id"]], "costume": equipped.get(h["id"]),
                 # A story frame of her (research/pick_pictures.py); no portrait art survived.
                 # Standing art: the official site's character art with its speech bubble removed
                 # and upscaled (research/stand_art.py); shown on the home screen.
                 # An equipped gacha outfit replaces it.
                 "stand_url": stand_url(h["id"], equipped.get(h["id"])),
                 # Square head crop of the standing art for the « » pills (research/fix_stand_alpha.py).
                 "face_url": (f"/static/{STATIC_VERSION}/img/heroine/{h['id']}_face.png"
                              if (STATIC_DIR / f"img/heroine/{h['id']}_face.png").is_file() else None),
                 "touch_id": TOUCH_ID.get(h["id"]),
                 "touch_line": TOUCH_SCENES.get(h["id"], {}).get("popup"),
                 # 82 voiced lines from the Japanese APK (sound/voice/mypage/<voice>.ogg), English where found.
                 "chat": [{"text": x["en"], "voice": x["voice"]} for x in TOUCH_LINES.get(h["id"], [])],
                 "costume_pool": [DEFAULT_COSTUME] + list(ALL_COSTUMES.get(h["id"], {})),
                 "bg": bgs.get(h["id"], DEFAULT_BG),
                 "face": faces.get(h["id"], DEFAULT_FACE),
                 "face_pool": [{"name": n, "url": f"/static/{STATIC_VERSION}/img/heroine/face/{FACES[h['id']]['faces'][n]}"
                                if n in FACES.get(h["id"], {}).get("faces", {}) else None} for n in FACE_NAMES],
                 "face_overlay": face_overlay(h["id"], equipped.get(h["id"]), faces.get(h["id"], DEFAULT_FACE)),
                 "bg_url": f"/static/{STATIC_VERSION}/{BACKGROUNDS[bgs.get(h['id'], DEFAULT_BG)]}",
                 # Her portrait from the post-story "Memory of ..." popup in the recordings.
                 "popup_url": f"/static/{STATIC_VERSION}/img/heroine/{h['id']}_popup.png"
                              if (STATIC_DIR / "img" / "heroine" / f"{h['id']}_popup.png").is_file() else None,
                 "costumes": owned.get(h["id"], []), "hidden": h["id"] == "06" and not misaki_open}
                for h in HEROINES]
    return {
        "invite_id": u["invite_id"],
        "name": u["name"],
        "heroine_id": u["heroine_id"],
        "crystals": u["crystals"],
        "items": {"gold_ticket": u["gold_tickets"], "silver_ticket": u["silver_tickets"],
                  "bronze_ticket": u["bronze_tickets"], "panel_ticket_dx": u["panel_tickets"]},
        "course": u["course"],
        "memory_pass": (u["course"] not in (None, "", "none")) or (u["pass_until"] or 0) > now(),
        "pass_until": u["pass_until"],
        "tickets": {k: tickets[k] for k in ("tickets", "max", "next_in")},
        "login_streak": u["login_streak"],
        "panels": {"revealed": u["panels"], "total": PANELS_PER_PICTURE, "completed": u["pictures_done"],
                   "current": photo_entry(PANEL_PHOTOS[u["pictures_done"] % len(PANEL_PHOTOS)]) if PANEL_PHOTOS else None},
        # Event pictures: the events can't be played (no text survives), so the recovered ones are simply shown.
        "photos": [photo_entry(p) for p in PANEL_PHOTOS[:u["pictures_done"]]]
                  + [photo_entry(p) for p in PHOTOS if p["kind"] == "event"]
                  + [photo_entry(p) for p in PHOTOS if p["kind"] == "epilogue" and
                     story_id(p["heroine_id"], int(p["title"][-1]), 1, arc="02") in read],
        "prologue_done": f"{STORY_PREFIX}0000000002" in read,
        "heroines": heroines,
        "backgrounds": [{"name": k, "url": f"/static/{STATIC_VERSION}/{v}" if v else None} for k, v in BACKGROUNDS.items()],
    }


# --------------------------------------------------------------------------- #
# WebView front-end
# --------------------------------------------------------------------------- #

@app.route("/")
def index():
    """Top page. MainActivity loads site_url + "/" (with ?__t=<ms>), then the SPA
    routes by hash: #/my/, #/tutorials/movie, ... The original front-end was never
    archived (it required auth), so templates/index.html is rebuilt from screenshots."""
    require_user()
    return render_template(
        "index.html",
        session=session_signer.dumps(g.user["uuid"]),
        first_story_id=FIRST_STORY_ID,
        state=player_state(),
    )


@app.route("/my/")
def my_page_json():
    """AppResourceLoader.requestInviteId(): GET site_url + "/my/" -> ProxyMyPageJson."""
    if not g.user:
        return proxy_json(None, error="unauthorized"), 401
    return proxy_json({"invite_id": g.user["invite_id"]})


# ---- mock JSON API used by templates/index.html (shapes are this mock's own) ---

@app.route("/api/me")
def api_me():
    require_user()
    return jsonify(player_state())


@app.route("/api/login_bonus", methods=["POST"])
def api_login_bonus():
    """Daily login removes one panel from a hidden picture (store listing)."""
    require_user()
    today = datetime.date.today().isoformat()
    if g.user["last_login_day"] == today:
        return jsonify(granted=False, state=player_state())
    panels, done = g.user["panels"] + 1, g.user["pictures_done"]
    if panels >= PANELS_PER_PICTURE:
        panels, done = 0, done + 1
    yesterday = (datetime.date.today() - datetime.timedelta(days=1)).isoformat()
    streak = g.user["login_streak"] + 1 if g.user["last_login_day"] == yesterday else 1
    db().execute("UPDATE users SET panels = ?, pictures_done = ?, last_login_day = ?, login_streak = ?, "
                 "login_days = login_days + 1 WHERE uuid = ?", (panels, done, today, streak, g.user["uuid"]))
    # JP login-bonus popup (神崎LEN, e.g. L04 0:59): every few logins an item — Silver Ticket or Panel
    # Ticket DX — "次のログインボーナスまではあと1回！". Mock: every 2nd login day, alternating.
    days, item = g.user["login_days"] + 1, None
    if days % 2 == 0:
        item = "silver_ticket" if days % 4 == 2 else "panel_ticket_dx"
        col = {"silver_ticket": "silver_tickets", "panel_ticket_dx": "panel_tickets"}[item]
        db().execute(f"UPDATE users SET {col} = {col} + 1 WHERE uuid = ?", (g.user["uuid"],))
    db().commit()
    reload_user()
    return jsonify(granted=True, item=item, next_in=1 if days % 2 else 2, state=player_state())


@app.route("/api/heroine/<heroine_id>", methods=["POST"])
def api_select_heroine(heroine_id):
    require_user()
    if heroine_id not in HEROINE_BY_ID:
        abort(404)
    db().execute("UPDATE users SET heroine_id = ? WHERE uuid = ?", (heroine_id, g.user["uuid"]))
    db().commit()
    reload_user()
    return jsonify(player_state())


@app.route("/api/events")
def api_events():
    """Memory > Event Story: the known events; the few with recovered text list their playable parts."""
    require_user()
    return jsonify(events=[{**e, "banner_url": f"/static/{STATIC_VERSION}/{e['banner']}" if e.get("banner") else None}
                           for e in EVENTS])


@app.route("/api/stories/<heroine_id>")
def api_stories(heroine_id):
    """Memory screen data: Prologue 1/2 + the heroine's chapters (grouped into
    parts) and epilogues, with read/locked flags."""
    require_user()
    if heroine_id not in HEROINE_BY_ID:
        abort(404)
    read = read_ids(g.user["uuid"])
    rows = [s for s in STORIES if s["heroine_id"] in ("00", heroine_id)]
    out = []
    for s in rows:
        prev = previous_story(s)
        out.append({**s, "read": s["scenario_id"] in read,
                    "locked": prev is not None and prev not in read and s["scenario_id"] not in read})
    return jsonify(stories=out)


@app.route("/api/story/<sid>/read", methods=["POST"])
def api_read_story(sid):
    """Spend 1 energy to read a new scenario part (epilogues and secret stories cost crystals instead).
    Re-reading from Memory is free.
    409 = not enough energy -> client shows the recover dialog."""
    valid_story_id(sid)
    require_user()
    story = STORY_BY_ID.get(sid) or abort(404)
    read = read_ids(g.user["uuid"])
    if sid not in read:
        prev = previous_story(story)
        if prev is not None and prev not in read:
            return jsonify(error="locked", message="Read the previous part first."), 403
        if story["kind"] in ("epilogue", "secret"):
            cost = EPILOGUE_COST if story["kind"] == "epilogue" else SECRET_COST
            if g.user["crystals"] < cost:
                what = "The epilogue" if story["kind"] == "epilogue" else f'"{story["title"]}"'
                return jsonify(error="crystals", message=f"{what} costs {cost} crystals. "
                               f"You need {cost - g.user['crystals']} crystal(s) more!"), 402
            db().execute("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", (cost, g.user["uuid"]))
            db().execute("INSERT OR IGNORE INTO story_read VALUES (?, ?, ?)", (g.user["uuid"], sid, now()))
            db().commit()
            reload_user()
            return jsonify(ok=True, state=player_state())
        t = ticket_state(g.user)
        if t["tickets"] < 1:
            return jsonify(error="no_energy", state=player_state()), 409
        # Spending from full starts the regen clock now.
        save_tickets(t["tickets"] - 1, t["anchor"] if t["tickets"] < t["max"] else now())
    db().execute("INSERT OR IGNORE INTO story_read VALUES (?, ?, ?)", (g.user["uuid"], sid, now()))
    db().commit()
    reload_user()
    return jsonify(ok=True, state=player_state())


@app.route("/api/energy/recover", methods=["POST"])
def api_recover_energy():
    """Shop > Recover Energy ("Restore one energy bar with 1 crystal") and the
    Item screen: Gold Ticket = full, Silver Ticket = +1, 4 Bronze Tickets = +1."""
    require_user()
    method = (request.get_json(silent=True) or {}).get("method", "gold_ticket")
    u = g.user
    if method == "crystal":
        if u["crystals"] < CRYSTALS_PER_ENERGY:
            return jsonify(error="not enough crystals"), 402
        db().execute("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", (CRYSTALS_PER_ENERGY, u["uuid"]))
        add_energy(1)
    elif method == "gold_ticket":
        if u["gold_tickets"] < 1:
            return jsonify(error="no Gold Tickets"), 402
        db().execute("UPDATE users SET gold_tickets = gold_tickets - 1 WHERE uuid = ?", (u["uuid"],))
        save_tickets(ticket_max(u), now())
    elif method == "silver_ticket":
        if u["silver_tickets"] < 1:
            return jsonify(error="no Silver Tickets"), 402
        db().execute("UPDATE users SET silver_tickets = silver_tickets - 1 WHERE uuid = ?", (u["uuid"],))
        add_energy(1)
    elif method == "bronze_ticket":
        if u["bronze_tickets"] < BRONZE_PER_ENERGY:
            return jsonify(error=f"need {BRONZE_PER_ENERGY} Bronze Tickets"), 402
        db().execute("UPDATE users SET bronze_tickets = bronze_tickets - ? WHERE uuid = ?",
                     (BRONZE_PER_ENERGY, u["uuid"]))
        add_energy(1)
    else:
        abort(400)
    db().commit()
    reload_user()
    return jsonify(ok=True, state=player_state())


@app.route("/api/shop/exchange/<item>", methods=["POST"])
def api_exchange(item):
    """Shop > Exchange items: Gold Ticket (5 crystals), Panel Ticket DX (3 crystals)."""
    require_user()
    price = ITEM_PRICES.get(item) or abort(404)
    if g.user["crystals"] < price:
        return jsonify(error="not enough crystals"), 402
    if item in PASS_DAYS:
        start = max(now(), g.user["pass_until"] or 0)
        db().execute("UPDATE users SET crystals = crystals - ?, pass_until = ? WHERE uuid = ?",
                     (price, start + PASS_DAYS[item] * 86400, g.user["uuid"]))
    else:
        col = {"gold_ticket": "gold_tickets", "panel_ticket_dx": "panel_tickets"}[item]
        db().execute(f"UPDATE users SET crystals = crystals - ?, {col} = {col} + 1 WHERE uuid = ?",
                     (price, g.user["uuid"]))
    db().commit()
    reload_user()
    return jsonify(ok=True, state=player_state())


@app.route("/api/panels/use_ticket", methods=["POST"])
def api_use_panel_ticket():
    """Panel Ticket DX removes one photo panel without waiting for a login."""
    require_user()
    if g.user["panel_tickets"] < 1:
        return jsonify(error="no Panel Ticket DX"), 402
    panels, done = g.user["panels"] + 1, g.user["pictures_done"]
    if panels >= PANELS_PER_PICTURE:
        panels, done = 0, done + 1
    db().execute("UPDATE users SET panel_tickets = panel_tickets - 1, panels = ?, pictures_done = ? WHERE uuid = ?",
                 (panels, done, g.user["uuid"]))
    db().commit()
    reload_user()
    return jsonify(ok=True, state=player_state())


@app.route("/api/name", methods=["POST"])
def api_name():
    """Player name shown in the story (default William)."""
    require_user()
    name = ((request.get_json(silent=True) or {}).get("name") or "").strip()[:12]
    if not name:
        abort(400)
    db().execute("UPDATE users SET name = ? WHERE uuid = ?", (name, g.user["uuid"]))
    db().commit()
    reload_user()
    return jsonify(player_state())


@app.route("/api/news")
def api_news():
    return jsonify(news=NEWS)


@app.route("/api/courses")
def api_courses():
    return jsonify(courses=COURSES)


@app.route("/api/soundtrack")
def api_soundtrack():
    """The official soundtrack for the in-app music room (templates/index.html). Each track carries a ready URL
    to its .ogg under the static tree; the home screen plays it with an HTML5 <audio>."""
    tracks = [{**t, "url": f"/static/{STATIC_VERSION}/{t['file']}"} for t in SOUNDTRACK]
    return jsonify(tracks=tracks)


@app.route("/api/gacha/<heroine_id>", methods=["POST"])
def api_gacha(heroine_id):
    """Crystals buy costume gacha pulls for the dress-up (着せ替え) mode."""
    require_user()
    if heroine_id not in HEROINE_BY_ID:
        abort(404)
    pool = list(COSTUMES.get(heroine_id, {}))
    if not pool:
        return jsonify(error=f"No outfits for {HEROINE_BY_ID[heroine_id]['short']} have been recovered yet."), 404
    if g.user["crystals"] < GACHA_COST:
        return jsonify(error="not enough crystals"), 402
    costume = random.choice(pool)
    db().execute("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", (GACHA_COST, g.user["uuid"]))
    cur = db().execute("INSERT OR IGNORE INTO costumes VALUES (?, ?, ?, 0)",
                       (g.user["uuid"], heroine_id, costume))
    duplicate = cur.rowcount == 0
    if duplicate:  # dupes refund a Gold Ticket instead
        db().execute("UPDATE users SET gold_tickets = gold_tickets + 1 WHERE uuid = ?", (g.user["uuid"],))
    db().commit()
    reload_user()
    return jsonify(costume=costume, duplicate=duplicate, url=stand_url(heroine_id, costume), state=player_state())


@app.route("/api/dress_bg/<heroine_id>", methods=["POST"])
def api_dress_bg(heroine_id):
    require_user()
    bg = (request.get_json(silent=True) or {}).get("bg")
    if heroine_id not in HEROINE_BY_ID or not BACKGROUNDS.get(bg):
        abort(404)
    db().execute("INSERT OR REPLACE INTO dress_bg (uuid, heroine_id, bg) VALUES (?, ?, ?)", (g.user["uuid"], heroine_id, bg))
    db().commit()
    return jsonify(player_state())


@app.route("/api/dress_face/<heroine_id>", methods=["POST"])
def api_dress_face(heroine_id):
    require_user()
    face = (request.get_json(silent=True) or {}).get("face")
    if heroine_id not in HEROINE_BY_ID or (face != DEFAULT_FACE and face not in FACES.get(heroine_id, {}).get("faces", {})):
        abort(404)
    db().execute("INSERT OR REPLACE INTO dress_face (uuid, heroine_id, face) VALUES (?, ?, ?)", (g.user["uuid"], heroine_id, face))
    db().commit()
    return jsonify(player_state())


@app.route("/api/costume/<heroine_id>", methods=["POST"])
def api_equip(heroine_id):
    require_user()
    costume = (request.get_json(silent=True) or {}).get("costume")
    base = card_base(heroine_id, costume)       # a card version is worn through its outfit's ownership
    owned = db().execute("SELECT 1 FROM costumes WHERE uuid = ? AND heroine_id = ? AND costume = ?",
                         (g.user["uuid"], heroine_id, base or costume)).fetchone()
    if not owned:
        abort(404)
    if base:
        db().execute("INSERT OR IGNORE INTO costumes VALUES (?, ?, ?, 0)", (g.user["uuid"], heroine_id, costume))
    db().execute("UPDATE costumes SET equipped = (costume = ?) WHERE uuid = ? AND heroine_id = ?",
                 (costume, g.user["uuid"], heroine_id))
    db().commit()
    return jsonify(player_state())


# --------------------------------------------------------------------------- #
# Story engine data (fetched by ResourceLoader before the native adventure scene)
# Order: offset json -> transcript -> resource_list -> static files
# --------------------------------------------------------------------------- #

@app.route("/resource/character_offset")
def character_offset():
    """app.json: offset_json_url_path. Client saves `content` to json/offset.json.
    Native ExOffsetData expects project "01" -> per-character offsets."""
    content = load_fixture(DATA_DIR / "character_offset.json") or {"01": {}}
    return proxy_json(content)


@app.route("/scenarios/<sid>/transcript")
def transcript(sid):
    """app.json: story_json_url_path. Client saves `content` to json/<id>.json and
    the native engine (ExScenarioData, picojson) plays it."""
    valid_story_id(sid)
    content = load_fixture(SCENARIO_DIR / sid / "transcript.json")
    if sid in TOUCH_BY_ID:
        content = TOUCH_SCENES[TOUCH_BY_ID[sid]]
    if content is None:
        content = default_transcript(sid)
    return proxy_json(with_player_name(engine_format(sid, content)))


def with_player_name(content):
    """The stories call the player William (recordings' custom names were normalised to it); a player who chose
    another name in More > Name sees theirs in the speaker box and the text."""
    name = g.user["name"] if g.user else None
    if not name or name == DEFAULT_PLAYER_NAME:
        return content
    pat = re.compile(rf"\b{DEFAULT_PLAYER_NAME}\b")
    for line in content["scenario_data"]:
        scene = line["scene"]
        for k in ("speaker", "body"):
            if isinstance(scene.get(k), str):
                scene[k] = pat.sub(lambda _m: name, scene[k])
    return content


def end_mark_type(sid):
    """"end" shows the Fin card (last scenario of a route), "continue" the
    to-be-continued card between parts."""
    if sid in TOUCH_BY_ID:
        return "none"       # _Ve8sVkG1XM: the touch scene goes straight back to the home screen
    if sid in EVENT_EPISODES:
        return "continue"
    if sid in EVENT_SCENARIOS:
        return "end"
    i = STORY_INDEX.get(sid)
    if i is None:
        return "continue"
    s = STORIES[i]
    if s["kind"] == "secret":
        return "continue"   # HDBarbecho AphjRVlp5gY 12:10: "To Be Continued"
    last = i + 1 >= len(STORIES) or STORIES[i + 1]["heroine_id"] != s["heroine_id"]         or STORIES[i + 1]["kind"] == "secret"
    return "end" if last and s["kind"] != "prologue" else "continue"


# Story pictures that came out solid black: the line's text appeared while the scene was still fading in, so its
# frame of the recording is black (research/black_frames.py, 115 of 9,874).
BLACK_FRAMES = set(load_fixture(DATA_DIR / "black_frames.json") or [])


def unblack(bgs):
    """Per-line picture names -> the same with each black one replaced by the part's next real picture (the scene
    fading in), or the previous one at the end of a part."""
    out, nxt = list(bgs), None
    for i in range(len(out) - 1, -1, -1):
        if out[i] in BLACK_FRAMES:
            out[i] = nxt
        elif out[i]:
            nxt = out[i]
    prev = None
    for i, bg in enumerate(out):
        if bg:
            prev = bg
        elif bgs[i] in BLACK_FRAMES:
            out[i] = prev
    return out


def engine_format(sid, content):
    """Convert stored transcripts to the layout libotome.so actually parses.

    Verified by decompiling ExStoryData::parse / ExScenarioData::parseJson (v1.4.6):
      {"scenario_id", "scenario_data": [
        {"scene":  {"speaker" ("__NULL__" = nobody), "body", "bg", "fg": [..] | "<str>",
                    "show_tail": auto|none|speak|think, "link_scene": "0"|"1"},
         "audio":  {"bgm", "se", "voice"},
         "visual": {"fade": {type,on,speed,color}, "camera": {type}, "filter": {type,on,option},
                    "manpu": {location,type,path}, "shake": {type}, "particle": {type,spec,path},
                    "end_mark": {"type": none|continue|end}}}]}
    fg entries: {character_id, app_id, perspective, size: s|m|l, position: l|m|r, layer}.
    Every visual sub-key present is read as an object, so absent effects must be
    omitted, never null. bg is a frame of the source recording (research/scene_frames.py);
    fg is left out because no sprite art has been recovered.
    Older files (speaker/body/show_tail at the top level of each line) are converted."""
    out = []
    for line in content.get("scenario_data", []):
        scene = line.get("scene") if isinstance(line.get("scene"), dict) else {}
        if "body" not in scene and "body" not in line:
            continue  # old-format {"end_mark": true} terminator
        speaker = scene.get("speaker", line.get("speaker")) or "__NULL__"
        tail = scene.get("show_tail", line.get("show_tail"))
        body = scene.get("body", line.get("body", ""))
        if not isinstance(tail, str):
            # Recordings: a heroine's (thoughts) use the yellow bubble box ("think").
            tail = ("think" if body.lstrip().startswith("(") else "speak") if tail else "none"
        new_scene = {"speaker": speaker, "body": body,
                     "show_tail": tail, "link_scene": "0"}
        for k in ("bg", "fg"):
            if scene.get(k):
                new_scene[k] = scene[k]
        entry = {"scene": new_scene}
        audio = {k: v for k, v in (line.get("audio") or {}).items() if isinstance(v, str) and v}
        if audio:
            entry["audio"] = audio
        visual = {k: v for k, v in (line.get("visual") or {}).items() if isinstance(v, dict)}
        if visual:
            entry["visual"] = visual
        out.append(entry)
    for entry, bg in zip(out, unblack([e["scene"].get("bg") for e in out])):
        if bg:
            entry["scene"]["bg"] = bg
        else:
            entry["scene"].pop("bg", None)
    # Recordings: a chapter's first part opens on the heroine's name card with the
    # title in the narration box ("Chapter 5 - A Helping Hand", "Epilogue 2").
    story = STORY_BY_ID.get(sid)
    if story and story["kind"] in ("main", "epilogue") and story["part"] == 1 \
            and not (out and out[0]["scene"]["body"] == story["title"]):
        title = {"speaker": "__NULL__", "body": story["title"], "show_tail": "none", "link_scene": "0"}
        # The title line shows the heroine's name card (taken from her Chapter 5 upload).
        if (STATIC_DIR / "img" / "resource" / "bg" / f"card_{story['heroine_id']}.png").is_file():
            title["bg"] = f"card_{story['heroine_id']}"
        elif out and out[0]["scene"].get("bg"):
            title["bg"] = out[0]["scene"]["bg"]
        out.insert(0, {"scene": title})
    # Music: the original per-scene tracks were downloads from the live server and are lost, so each story opens on
    # its heroine's character theme from the official soundtrack (HEROINE_BGM; the shared prologue and heroine-less
    # stories use the main theme). The engine passes "Application/cache/" + audio.bgm to AudioProcessor.playBgm,
    # which opens a path without a leading "/" from the APK's assets and loops it; lines without audio.bgm keep it
    # playing. The offline APK carries every soundtrack file there (offline/build_apk.py); the original APK doesn't,
    # so on the PC server stories stay silent (the engine catches the FileNotFoundException).
    if out and not any(e.get("audio", {}).get("bgm") for e in out):
        story_bgm = HEROINE_BGM.get((story or {}).get("heroine_id"), MAIN_BGM)
        out[0].setdefault("audio", {})["bgm"] = story_bgm
    if out and "end_mark" not in out[-1].get("visual", {}):
        out[-1].setdefault("visual", {})["end_mark"] = {"type": end_mark_type(sid)}
    return {"scenario_id": content.get("scenario_id", sid), "scenario_data": out}


def scene_pictures(sid):
    """Background pictures a transcript uses (scene.bg -> img/resource/bg/<bg>.png; the
    engine only loads PNG). The client downloads each into its cache before playing."""
    content = load_fixture(SCENARIO_DIR / sid / "transcript.json") or TOUCH_SCENES.get(TOUCH_BY_ID.get(sid), {})
    story = STORY_BY_ID.get(sid) or {}
    seen = []
    if story.get("kind") in ("main", "epilogue") and story.get("part") == 1             and (STATIC_DIR / "img" / "resource" / "bg" / f"card_{story['heroine_id']}.png").is_file():
        seen.append(f"/img/resource/bg/card_{story['heroine_id']}.png")
    lines = content.get("scenario_data", [])
    for bg in unblack([(line.get("scene") if isinstance(line.get("scene"), dict) else {}).get("bg") for line in lines]):
        if isinstance(bg, str) and bg and f"/img/resource/bg/{bg}.png" not in seen                 and (STATIC_DIR / "img" / "resource" / "bg" / f"{bg}.png").is_file():
            seen.append(f"/img/resource/bg/{bg}.png")
    return seen


@app.route("/scenarios/<sid>/resource_list")
def resource_list(sid):
    """app.json: resource_list_json_url_path -> ProxyResourceListJson.
    Each entry in `resources` is downloaded from
        <static_hostname>/<static_version><path>
    and cached at <path>. .zip entries are unzipped with the app.json zip_file_password.
    An empty list is fine: NetworkClient.requestParallel() succeeds immediately."""
    valid_story_id(sid)
    # next_scenario_id_list lets the client prefetch the following part in the background.
    i = STORY_INDEX.get(sid)
    nxt = [STORIES[i + 1]["scenario_id"]] if i is not None and i + 1 < len(STORIES)         and STORIES[i + 1]["heroine_id"] == STORIES[i]["heroine_id"] else []
    content = load_fixture(SCENARIO_DIR / sid / "resource_list.json") or {
        "resources_id": sid,
        "resources": scene_pictures(sid),
        "next_scenario_id_list": nxt,
    }
    return proxy_json(content, static_hostname=site_url() + "/static",
                      static_version=STATIC_VERSION)


@app.route("/static/<int:version>/<path:path>")
def static_resource(version, path):
    """Binary assets (img/resource/bg/..., img/resource/character/..., sound, zips)."""
    return send_from_directory(STATIC_DIR, path)


@app.route("/debug/main_story/list")
def main_story_list():
    """Only reachable from the debug build's debug scene -> ProxyMainStoryListJson."""
    return proxy_json([
        {k: s.get(k) for k in ("scenario_id", "title", "chapter", "partner_name", "story_type")}
        for s in STORIES
    ])


def default_transcript(sid):
    """Placeholder for parts with no recovered text yet (engine_format adds the
    title line and end mark)."""
    story = STORY_BY_ID.get(sid, {})
    heroine = HEROINE_BY_ID.get(story.get("heroine_id"))
    if heroine:
        # English edition conventions: short speaker name, "double quotes".
        quote = heroine.get("card", {}).get("quote", heroine["quote"])
        lines = [(heroine["short"], f'"{quote}"', "speak")]
    else:
        lines = [("__NULL__", "Your apartment was wrecked by a truck. Aoi brings you to her share house...", "none")]
    lines.append(("__NULL__", "(This part's text hasn't been recovered yet.)", "none"))
    data = [{"scene": {"speaker": sp, "body": body, "show_tail": tail, "link_scene": "0"}}
            for sp, body, tail in lines]
    return {"scenario_id": sid, "scenario_data": data}


# --------------------------------------------------------------------------- #
# Payments (Google Play IAB v3). These are POSTed by Volley, not the WebView.
# --------------------------------------------------------------------------- #

def crystals_for(product_id):
    # jp.mfapps.novel.roomies.item.credit.<N>[.sale[.beginner]]
    m = re.search(r"\.credit\.(\d+)", product_id or "")
    return int(m.group(1)) if m else 0


def grant(product_id, raw):
    if not g.user:
        return
    db().execute("UPDATE users SET crystals = crystals + ? WHERE uuid = ?",
                 (crystals_for(product_id), g.user["uuid"]))
    db().execute("INSERT INTO purchases (uuid, product_id, raw, created_at) VALUES (?, ?, ?, ?)",
                 (g.user["uuid"], product_id, raw, now()))
    db().commit()


@app.route("/payment/android/purchase", methods=["POST"])
@app.route("/payment/android/subscribe", methods=["POST"])
def payment_post():
    """AppPaymentPostClient.checkPurchaseStatus(): form field purchase_data is the
    Play purchase originalJson, encrypted with the same AES scheme. HTTP 200 = OK.
    The mock trusts it; a real server must verify the Play signature server-side."""
    token = request.form.get("purchase_data", "")
    try:
        purchase = json.loads(yoga_crypto.decrypt(app.config["AES_KEY"], token))
    except Exception as exc:
        log.warning("bad purchase_data: %s", exc)
        return jsonify(error="bad purchase_data"), 400
    log.info("    purchase: %s", purchase)
    grant(purchase.get("productId"), json.dumps(purchase))
    return jsonify(result="ok")


@app.route("/native/purchase")
def purchase_dummy():
    """/native/purchase_dummy in the WebView makes the app GET
    site_url + /native/purchase?product_id=..&callback_url=.. (with Yoga headers)."""
    product_id = request.args.get("product_id", "")
    grant(product_id, json.dumps(dict(request.args)))
    return "__SUCCESS__"


# --------------------------------------------------------------------------- #
# /native/* bridge stubs for testing the front-end in a desktop browser.
# On a device these never reach the server: JsViewClient.shouldInterceptRequest
# answers them locally. (Except /native/purchase, handled above.)
# --------------------------------------------------------------------------- #

NATIVE_SYNC_STUBS = {
    "get_api_version": str(API_VERSION),
    "get_app_info": json.dumps({"package_name": "jp.mfapps.novel.roomies",
                                "version_code": 0, "version_name": "browser"}),
    "get": "",
    "get_price": "",
    "get_purchase_data": "[]",
    "noah_offer_enable": "false",
    "tapjoy_offer_enable": "false",
}


@app.route("/native/<name>")
def native_stub(name):
    log.info("    (browser) native bridge call %s %s", name, dict(request.args))
    return NATIVE_SYNC_STUBS.get(name, "__SUCCESS__")


# --------------------------------------------------------------------------- #
# Catch-all: log everything we don't know yet.
# --------------------------------------------------------------------------- #

@app.route("/<path:path>", methods=["GET", "POST", "PUT", "DELETE"])
def unknown(path):
    log.warning("    UNHANDLED %s /%s form=%s", request.method, path, dict(request.form))
    # The native title screen hands the WebView a URL we can't see in Java;
    # top-level page loads come from Volley with Yoga headers, so give them the SPA.
    if request.method == "GET" and "X-Yoga-Authorization" in request.headers \
            and "json" not in request.headers.get("Accept", ""):
        return index()
    return proxy_json(None, error=f"unhandled path /{path}"), 404


@app.route("/__mock__/maintenance/<state>", methods=["POST"])
def toggle_maintenance(state):
    """Flip is_maintenance=1 on every ProxyJson to see how the client reacts."""
    app.config["MAINTENANCE"] = state == "on"
    return jsonify(maintenance=app.config["MAINTENANCE"])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=80,
                        help="80 lets a hosts-file redirect work without touching the APK")
    parser.add_argument("--apk", help="derive the AES key from this APK's signing cert "
                                      "(needed if you re-signed the APK)")
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s", datefmt="%H:%M:%S")
    logging.getLogger("werkzeug").setLevel(logging.WARNING)

    if args.apk:
        app.config["AES_KEY"] = yoga_crypto.key_from_apk(args.apk)
    log.info("AES key: %s", app.config["AES_KEY"].hex())

    init_db()
    app.run(host=args.host, port=args.port, debug=args.debug)


init_db()

if __name__ == "__main__":
    main()
