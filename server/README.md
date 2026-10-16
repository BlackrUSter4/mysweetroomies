# My Sweet Roomies! — educational mock server

A Flask server that speaks the client protocol of **My Sweet Roomies!**
(`jp.mfapps.novel.roomies` v1.4.6). The game is the English edition of Fuji TV's
**「ようこそ！ファミーユへ」 (Welcome to Famille!)**, `jp.mfapps.novel.famille`. It ran from
29 Jul 2014 to 13 Nov 2019, when Fuji・Nextera Lab shut both editions down.

Sources:
- **Decompiled APK:** both editions, via jadx, plus strings from `libotome.so`.
- **Archived pages:** the official site and the game server's one capture, from the Wayback Machine.
- **Store listings and reviews:** TapTap, applion, dotapps, appget, 4gamer, gamebiz.
- **Store screenshots** of the original UI.

```
pip install -r requirements.txt
python app.py                 # listens on 0.0.0.0:80
python app.py --port 8080     # any port
python app.py --apk ../My+Sweet+Roomies!_1.4.6_APKPure.apk   # derive the AES key from an APK
python -m pytest test_server.py
```

To try the UI in a desktop browser, open `http://localhost/?dev=1`. That parameter fakes a
device. Without it, the server answers `401 Unauthorized`, which is exactly what the real
server did.

## Pointing the game at it

Production was `http://android.roomies.novel.mfapps.jp` over **plain HTTP**. The Japanese
edition used `android.famille.novel.mfapps.jp`. Pick one of these:

1. **hosts redirect (no APK changes).** On a rooted emulator:
   `adb root && adb remount && adb shell "echo 10.0.2.2 android.roomies.novel.mfapps.jp >> /etc/hosts"`.
   `10.0.2.2` is the host PC as seen from the emulator. Run the server on port 80.
2. **mitmproxy map-remote.** Set the device's Wi-Fi proxy to your PC, then run
   `mitmproxy --map-remote "|//android.roomies.novel.mfapps.jp|//127.0.0.1:8080"`.
3. **Patch the APK.** Edit `assets/Application/document/json/product/android/app.json` and
   re-sign. Re-signing changes the AES key, so start the server with `--apk <re-signed apk>`.

## How the client works

```
Cocos2d-x title screen (native, offline)
   └─ "Start" ─► WebView loads site_url + "/"            ← templates/index.html
                   │  SPA hash routes: #/my/  #/tutorials/movie  ...
                   │  calls the app via  /native/<method>?a=..&b=..   (intercepted in Java)
                   └─ /native/start_story?story_id=X&callbackurl=/my/
                        └─ ResourceLoader, with Yoga headers:
                             GET /resource/character_offset
                             GET /scenarios/X/transcript
                             GET /scenarios/X/resource_list
                             GET <static_hostname>/<static_version>/<each resource>
                        └─ native adventure scene plays json/X.json, then returns to #/my/
```

### Headers on every native-originated request (`YogaRequestBuilderFactory`)

| Header | Value |
|---|---|
| `X-Yoga-Authorization` | encrypted `{"device_id","timestamp","uuid"}`. The `uuid` is generated once per install and acts as the account ID |
| `X-Package-Name`, `X-Version-Code`, `X-Version-Name` | from `PackageInfo` |
| `X-Build-Info` | `{"sdk_int","release","model","id"}` |
| `X-Yoga-Request-From` | `native` |
| `X-Yoga-Kpi-ID` | sha256(first Google account email + fixed salt) |

The top-level page is fetched by **Volley**, not by the WebView, so only it carries these
headers. That's why the server's crawled top page is a 401: crawlers had no auth header. The
page's own XHRs carry a signed `X-Mock-Session` token instead.

### Crypto (`util/Security.java`)

AES-128-CBC with PKCS5 padding. IV `0123456789abcdef`. Output is URL-safe Base64 without padding.
The key is `MD5(hex(DER signing certificate))`, which for the original Fuji TV signature is
`e9ee6770731a5c5b7dff9b7f827cadb8`. The same scheme encrypts `purchase_data` on payment POSTs.

### Response envelope (`ProxyJson`)

```json
{"api_version":5,"timestamp":0,"version":1,"cache_lifetime":0,
 "is_maintenance":0,"error":null,"location":null,
 "static_hostname":"...","static_version":1,
 "content": { ... }}
```

`is_maintenance: 1` or a non-null `error` makes the client treat the response as a failure.
To try it, run `curl -X POST localhost/__mock__/maintenance/on`.

## Game data recovered from archives

**Heroines** (`data/heroines.json`). The IDs come from matching the official site's voice-sample
players to the heroine sections they sit in:

| ID | Heroine | CV | Notes |
|---|---|---|---|
| 01 | Ren Kiryu 霧生 蓮 | Saori Onishi | 20, the cool "untouchable beauty" |
| 02 | Aki Momohara 桃原 アキ | Kiyono Yasuno | 18, pop idol |
| 03 | Mitsuka Ibuki 伊吹 みつか | Ibuki Kido | 18, youngest, shy |
| 04 | Aoi Mizusawa 水沢 あおい | Azusa Tadokoro | 21, best friend who brings you in |
| 05 | Sayaka Shimono 下野 さやか | Shizuka Ishigami | 20, No.1 hostess |
| 06 | Misaki Fujinaga 藤永 美咲 | Fumi Morisawa | 29, secretary and landlady |

**ID formats:**
- **Voice files:** `13` + speaker + route + 5-digit line. For example, `13020400012` is Aki
  speaking in Aoi's route.
- **Story IDs:** 12 digits. The client reads the partner from `[2:4]` and the chapter from `[8:10]`.
  The Prologue is two separately loaded scenarios, `530000000001` and `…02`, so `[10:12]` is the part.
- **Edition prefixes:** the first story is `130000000001` in Famille and `530000000001` in Roomies.

**Routes** (`data/chapter_titles.json`, `data/chapter_parts.json`). The sources:
- Every chapter of every route was uploaded to YouTube (2015–2017).
- Each chapter opens on a title card, which gives its title.
- Part counts come from the loading screens between parts in each chapter video.

| Heroine | Chapters | Titles | Story parts |
|---|---|---|---|
| Ren | 53 | 53 | 229 |
| Aki | 56 | 56 | 250 |
| Mitsuka | 52 | 51 | 227 |
| Aoi | 54 | 53 | 233 |
| Sayaka | 52 | 51 | 302 |
| Misaki | 54 | 54 | 247 |

That's 318 of 321 titles and 1,488 parts in total.

**The game's chapter numbering has gaps:**
- **Aoi** has no Chapter 27 or 32. A continuous playthrough goes 26 → 28 and 31 → 33.
  She also has two different chapters both numbered "34".
- **Aki** has no Chapter 36. This is inferred: no card and no upload exists, the same pattern
  as Aoi's.

**Other route facts:**
- Every route ends with Epilogue 1 and 2, which are usually a single part each.
- Chapters run from 2 to 11 parts, mostly 3 to 5, and each part costs 1 energy.
- **Check:** Aki Chapter 11 measures 7 parts, which matches a player's own notes
  ("home party 1-7").

**Prologue** (`data/scenarios/530000000001` and `…02`): the real English text, 160 lines,
transcribed from a 2019 recording. The protagonist's name defaults to **William** and can be
changed. Heroine lines use a pink box with a speech tail (`show_tail`); narration and the
protagonist use a cyan box without one. A route ends on a "Fin" card.

**Mechanics** (from recordings of the real shop screens, 2019):
- **Energy:** max 8, regaining 1 every 4 hours. Reading a part costs 1.
- **Recover Energy:** "Restore one energy bar with 1 crystal."
- **Items:** a Gold Ticket refills all energy, a Silver Ticket restores 1, and 4 Bronze
  Tickets restore 1. Bronze Tickets were free to earn until 2019/09/04. There is also a
  Panel Ticket DX.
- **Exchange items:** Gold Ticket = 5 crystals, Panel Ticket DX = 3 crystals.
- **Crystal Shop:**

  | Pack | Price | SKU |
  |---|---|---|
  | 1 | $0.99 | `credit.1` |
  | 5+1 | $4.21 | `credit.6` |
  | 12+3 | $10.10 | `credit.15` |
  | 20+6 | $16.87 | `credit.26` |
  | 28+12 | $23.56 | `credit.40` |
  | 42+26 | $35.29 | `credit.68` |
  | 78+57 | $65.64 | `credit.135` |

- **Monthly Courses:** Standard ($2.78) unlocks all memories. Special raises the energy max to 8.
  Delux cuts regen to 3 hours.
- **Menu:** News, Present, Photo, Item, Shop, Gacha, Dress Up, Character, Memory, More, Inquiries.
- **Memory:** tabs Main Story / Secret Story / Epilogue / Event Story.
- **Touching the heroine:** plays a short scene, then shows an "EXP %" line.
- **News** (`data/news.json`): five real posts from 2019, including the full closure notice.
- **Maintenance:** the screen reads "Under Maintenance – Please check back later." with a
  "Close the app." button.

**Back-office vocabulary** (from archived admin URLs, which are all behind a login):
- **Scene tag types:** `tag_builder/bg_tag`, `fg_tag`, `mob_tag`, `still_tag`, `preview`.
- **Event stories:** `story/scratch_event_story`.

## Confirmed vs. guessed

| Part | Status |
|---|---|
| URLs, headers, crypto, envelope, resource-list format, payment POST, `/native/*` bridge | **confirmed** from Java code |
| `401 Unauthorized` without auth | **confirmed** (Wayback capture, 2017-06-02) |
| Heroine IDs, names, profiles, voice-ID format | **confirmed** (official site and Famille APK) |
| Energy rules, item effects and prices, crystal packs, courses, menu, Memory tabs, news | **confirmed** (gameplay recordings) |
| Chapter list per route, 318 of 321 titles, parts per chapter | **confirmed** (title cards, loading-screen counts) |
| Prologue text | **confirmed** (transcribed); possibly 1 line missing around 5:56–6:05 |
| Epilogue story IDs, Special Course behaviour (base max already 8), whether the Prologue costs energy | **guessed** |
| Gacha price (5), costume pool, Secret/Event story lists | **guessed** or empty |
| Transcript layout (`scene`/`audio`/`visual`, enum values) | **confirmed** by decompiling `libotome.so` and playing stories in the real app |
| Web front-end HTML/JS and `/api/*` | **rebuilt** to match recorded screens (Home, MENU, Memory, Character, post-story popup); the original HTML was never archived |
| Tested with the real app | **yes**: APK 1.4.6 on an Android 12 emulator, stories play end to end |
| Character art | **not included** |

## Research tools (`../research/`)

- **`frames.py`:** samples video frames into contact sheets.
- **`lines.py`:** extracts each settled dialogue box from a story video. This is how the
  prologue was transcribed.
- **`grab.sh`, `chapter_videos.txt`, `titles.txt`, `build_titles.py`:** collect the chapter
  title cards and turn them into `chapter_titles.json`.
- **`count_parts.sh`, `parts.py`, `compile_parts.py`:** count the loading screens in each
  chapter video and produce `chapter_parts.json`.
- **`afterload.py`:** takes the frame after every loading screen in a long recording. That's
  how the skipped chapter numbers were found.
- **Downloading:** YouTube may require sign-in ("confirm you're not a bot"). Put a Netscape
  `cookies.txt` next to the scripts; some videos only work without cookies, and the scripts
  try both.

## Using real data

Real responses override the generated defaults:

```
data/stories.json                         # list of {scenario_id, heroine_id, kind, chapter, part, title}
data/scenarios/<id>/transcript.json      # the "content" object only
data/scenarios/<id>/resource_list.json   # {"resources":[...], "next_scenario_id_list":[...], "resources_id":"..."}
data/character_offset.json
data/static/<path from resources list>   # e.g. data/static/img/resource/bg/xxx.png
```

Zip resources are unzipped on-device with the password from `app.json`
(`zip_file_password`).

Any request the server doesn't recognise is logged as `UNHANDLED`. That log is your to-do
list for whatever the reconstruction missed.
