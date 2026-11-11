# My Sweet Roomies! — Server Reconstruction

*As of 2026-10-14*

The mock server in `server/` reproduces the full client protocol of **My Sweet Roomies! v1.4.6**
(`jp.mfapps.novel.roomies`), plus real game data recovered from archives and 2014–2019
playthrough videos (English and Japanese). It has:
- all 323 chapter titles (3 translated from Japanese) and story text for every main-story and
  epilogue part of all six routes, with pictures taken from the recordings
- the real shop, energy, login-streak and login-bonus rules, and the two-half route structure
- 56 of 60 secret stories by title and unlock point (text only for Aki's first)
- the Photo window with real pictures, the gacha and Dress Up screens with 145 real outfits (23–26 per heroine),
  touch scenes for three heroines and touch chat lines for all six

**Tested with the real app** (APK 1.4.6 on a MuMu emulator, Android 12, 2026-10-09): it logs in,
shows the home screen, plays stories in the native story engine to the end card, and returns
to the home screen. See section 2a for what that testing fixed. Story text comes from OCR of
playthrough videos (section 4).

| | |
|---|---|
| Run | `cd server && python app.py` (port 80) · `--port 8080` · `--apk <apk>` for a re-signed APK |
| Offline APK | `offline/MySweetRoomies_offline.apk` (955 MB, the server inside; see section 5a) |
| Repository | https://github.com/BlackrUSter4/mysweetroomies (public, AGPL-3.0; APK under Releases v1.0) |
| Try in a browser | `http://localhost/?dev=1` |
| Tests | `python -m pytest test_server.py` (34 pass) |
| Research scripts + raw data | `research/` |

---

## 1. The game

- **What it was:** the English edition of Fuji TV's 「ようこそ！ファミーユへ」 (Welcome to Famille!,
  `jp.mfapps.novel.famille`), a full-voice dating sim about six women in a share house.
- **Dates:** Japanese launch 29 Jul 2014, English edition from Jan 2015. Both shut down on
  13 Nov 2019 (Fuji・Nextera Lab).
- **Architecture:** a hybrid app. A Cocos2d-x shell (`libotome.so`) holds the title screen and
  the story engine. Everything else is a web page the server sends to a WebView. That front-end
  was never archived (it required auth), so `server/templates/index.html` is rebuilt from
  recordings.

## 2. Client protocol (confirmed from the decompiled APK)

### Hosts
- **Production:** `http://android.roomies.novel.mfapps.jp` (plain HTTP). The Japanese edition
  used `android.famille.novel.mfapps.jp`.
- **Pointing the app at the mock:**
  - a hosts entry (rooted emulator, port 80)
  - mitmproxy `--map-remote`
  - patching `assets/Application/document/json/product/android/app.json` and re-signing

### Headers on every native request (`YogaRequestBuilderFactory`)

| Header | Value |
|---|---|
| `X-Yoga-Authorization` | encrypted `{"device_id","timestamp","uuid"}`. The `uuid` is the account ID |
| `X-Package-Name`, `X-Version-Code`, `X-Version-Name` | from `PackageInfo` |
| `X-Build-Info` | `{"sdk_int","release","model","id"}` |
| `X-Yoga-Request-From` | `native` |
| `X-Yoga-Kpi-ID` | sha256(Google account email + salt) |

The top-level page is fetched by Volley, so only it carries these headers. The WebView's own
XHRs don't, so the mock page sends a signed `X-Mock-Session` token. Without auth, the real
server answered **`401 Unauthorized`** (the Wayback capture from 2017-06-02), and the mock does
the same.

### Crypto (`util/Security.java`)

| | |
|---|---|
| Cipher | AES-128-CBC, PKCS5 padding |
| IV | `0123456789abcdef` |
| Encoding | URL-safe Base64, no padding |
| Key | `MD5(hex(DER signing certificate))` |
| Key, original signature | `e9ee6770731a5c5b7dff9b7f827cadb8` |

The same scheme encrypts `purchase_data` on payment POSTs. A re-signed APK has a different key,
so start the server with `--apk`.

### Response envelope (`ProxyJson`)
```json
{"api_version":5,"timestamp":0,"version":1,"cache_lifetime":0,"is_maintenance":0,
 "error":null,"location":null,"static_hostname":"…","static_version":1,"content":{…}}
```
`is_maintenance: 1` or a non-null `error` makes the client fail the request. The mock can
toggle it: `POST /__mock__/maintenance/on`.

### Story pipeline
```
title screen (native) ─► WebView loads site_url + "/"            (hash routes #/my/ …)
   └─ /native/start_story?story_id=X&callbackurl=/my/            (intercepted in Java)
        └─ GET /resource/character_offset
           GET /scenarios/X/transcript
           GET /scenarios/X/resource_list  → static_hostname/static_version/<path> downloads
        └─ native engine plays json/X.json, then returns to the callback URL
```
- **The `/native/*` bridge:** about 40 methods (start_story, play_movie, toast, purchase,
  purchase_dummy, show_option_menu, go_top, …). Arguments are mapped **by position in the
  query string, not by name**.
- **Payments:** `POST /payment/android/purchase` (and `/subscribe`), with form field
  `purchase_data` encrypted as above. The dummy purchase is `GET /native/purchase?product_id=…`.

### Story IDs
- **Format:** 12 digits, `53` + heroine + `0101` + chapter + part. The client reads the heroine
  from `[2:4]` and the chapter from `[8:10]`.
- **Prologue:** `530000000001` (the `first_story_id`) and `…02`. Its two separate loads confirm
  that `[10:12]` is the part.
- **Japanese edition:** uses the prefix `13`.
- **Transcript JSON:** see section 2a. The layout is confirmed by decompiling the parser.

## 2a. Tested against the real app (2026-10-09)

**Setup.** MuMu emulator with root (`adb connect 127.0.0.1:7555`). Install the APK, then add
`10.0.2.2 android.roomies.novel.mfapps.jp` to `/system/etc/hosts` (`su`, remount `/` rw). Run the
server on port 80. The original signature is kept, so the default AES key works.

**Fixes the real client needed:**

1. **No trailing newline on JSON.** `NetworkClient.isJsonResponse()` requires the body to *end*
   with `}` or `]`, and its regex `^\s*(\{|\[).*(\}|\])\s*$` runs without DOTALL. Flask's
   `jsonify` appends `\n`, so the client rejected every native response ("Connection Error").
   `app.py` strips it in an `after_request` hook.
2. **Transcript layout.** Decompiled `ExStoryData::parse` and `ExScenarioData::parseJson` in
   `libotome.so` with Ghidra 12.1. The binary keeps its symbols. String keys are
   PC-relative, resolved with a small script. The layout:

   ```json
   {"scenario_id": "530401010101",
    "scenario_data": [
      {"scene":  {"speaker": "Aoi", "body": "\"…\"", "show_tail": "speak", "link_scene": "0",
                  "bg": "<img/resource/bg/…>", "fg": [{"character_id","app_id","perspective",
                  "size": "s|m|l", "position": "l|m|r", "layer"}]},
       "audio":  {"bgm": "…", "se": "…", "voice": "…"},
       "visual": {"fade": {"type","on","speed","color"}, "camera": {"type"},
                  "filter": {"type","on","option"}, "manpu": {"location","type","path"},
                  "shake": {"type"}, "particle": {"type","spec","path"},
                  "end_mark": {"type": "none|continue|end"}}}]}
   ```

   - **`speaker`:** `"__NULL__"` means no name, as in narration.
   - **`show_tail`:**
     - `speak`: pink box with a tail, for heroines.
     - `none`: cyan box, for narration and the player.
     - `think`: yellow box with bubble dots, for a heroine's `(thoughts)`.
     - `auto`: also valid.
   - **Optional keys:** every key under `visual` that is present is read as an object, so a
     `null` crashes. Leave absent effects out.
   - **`end_mark`:**
     - `continue` shows the "To Be Continued" card.
     - `end` shows "Fin".
   - **Enum strings:** come from the `EnumString<…>` static initialisers.
   - **Old files still work:** `app.py: engine_format()` converts the older stored layout when
     serving. It also adds the chapter-title narration line that opens each chapter's first
     part ("Chapter 5 - A Helping Hand", seen in recordings).
3. **Return from a story.** The app loads `site_url + "/#" + callbackurl`. The page passes
   `/my/after/<story id>` and shows the post-story popup when it comes back.

**Verified on the device:**
- **Login:** the auth header decrypts and the account is created.
- **Story chain:** offset, transcript, resource list, prefetch of the next part.
- **Dialogue boxes:** all three types render.
- **End cards:** To Be Continued.
- **Return trip:** back to the page, then the "EXP is now x%!" popup.
- **Screens:** Memory, Memory Pass and Character.
- **Text:** a passage of Mitsuka Chapter 1 was compared line by line with the source video
  (section 4).

## 3. Game data recovered

### Heroines (IDs from the official site's voice samples)

| ID | Heroine | CV | Profile |
|---|---|---|---|
| 01 | Ren Kiryu 霧生 蓮 | Saori Onishi | 20, cool "untouchable beauty", wants to be a doctor |
| 02 | Aki Momohara 桃原 アキ | Kiyono Yasuno | 18, pop idol |
| 03 | Mitsuka Ibuki 伊吹 みつか | Ibuki Kido | 18, youngest, high schooler |
| 04 | Aoi Mizusawa 水沢 あおい | Azusa Tadokoro | 21, best friend, brings you to the house |
| 05 | Sayaka Shimono 下野 さやか | Shizuka Ishigami | 20, No.1 hostess, later studies nursing |
| 06 | Misaki Fujinaga 藤永 美咲 | Fumi Morisawa | 29, secretary and landlady |

**Voice files** follow `13` + speaker + route + 5-digit line. For example, `13020400012` is Aki
speaking in Aoi's route.

### Routes (`server/data/chapter_titles.json`, `chapter_parts.json`)

| Heroine | Chapters | Titles found | Story parts (main) |
|---|---|---|---|
| Ren | 53 | 53 | 229 |
| Aki | 57 | 57 | 251 |
| Mitsuka | 52 | 52 | 227 |
| Aoi | 55 | 55 | 238 |
| Sayaka | 52 | 52 | 302 |
| Misaki | 54 | 54 | 246 |

(Counts from the JSON files as of 2026-10-12, after adding Aoi 27 and Aki 36. Plus 2 epilogues
per route, the 2-part Prologue and 56 secret stories.)

**Chapter numbering (corrected 2026-10-12):**
- **Aoi** has a Chapter 27 "The Nearing End" (5 parts; JP 27話「近づく終わり」) and Aki a Chapter 36
  "Platinum Idol Award" (2 parts; JP 36話「プラチナアイドル賞！」). Both were uploaded by DevilGh0stz
  (`-qNRpvN6d3k`, `eZTOgnIpL4M`) but had been left out of `chapter_videos.txt`, so the earlier
  "no 27 / no 36" conclusion was wrong. They're now OCR'd and in the mock.
- Aoi's in-game title cards do skip 32: in-game "33 Crossing Point" = JP 32, "34 Overcoming It All" =
  JP 33, "34 A New Routine" = JP 34 (two "34"s); from 35 on the numbers agree again.
- Each route has **two halves**, each ending in an epilogue (section 4 "Fourth pass").

**Parts:** 2 to 11 per chapter, mostly 3 to 5. Each part loads separately and costs 1 energy.
Every route ends with Epilogue 1 and 2, usually a single part each.

**Missing titles: none since 2026-10-12.** The three English title cards that were never uploaded were translated from the Japanese edition (marked `title_source` in the JSON): Mitsuka 13 「ふたりの水着ショー」 "The Swimsuit Show for Two" (JP wiki), Aoi 54 「おれたちの幸せ」 "Our Happiness" (神崎LEN `-LaouuW06Z4` 3:01) and Sayaka 48 「つかの間の平穏」 "A Fleeting Calm" (神崎LEN `yfBCq5nxyck` 2:37). These are my translations, not the game's. Their only uploads start after the title
card.

### Mechanics (from 2019 recordings of the real shop screens)
- **Energy:** max 5 (8 with the Special or Delux course), regaining 1 every 4 hours. Logging in
  on consecutive days raises the max: 6 after 2 days, 7 after 5, 8 after 8; a missed day drops
  it back to 5 (section 4 "Second pass"). Reading a part costs 1. The "How to Play" screen shown after the Prologue says "0/5 → 1/5" and
  "Recover 1 energy every 4 hours!" (LaugherGalaxy `Eb07iH2w5L8` 35:52). The mock used 8
  before 2026-10-10.
- **Recover Energy:** 1 crystal restores 1 energy.
- **Crystal prices of content (JP, section 4 "Fourth pass"):** gacha pull 5, secret story 5,
  limited picture 10, epilogue 10. The mock charges crystals for epilogues and gacha; secret
  stories cost energy (as Aki's did in English).
- **Login bonus:** each daily login removes one of 16 photo panels; every few logins also gives
  a Silver Ticket or Panel Ticket DX (mock: every 2nd login, alternating).
- **Items:**
  - Gold Ticket: refills all energy.
  - Silver Ticket: +1 energy.
  - Bronze Tickets: 4 = +1 energy. They were free to earn until 2019-09-04.
  - Panel Ticket DX: removes one photo panel.
- **Exchange items:** Gold Ticket = 5 crystals, Panel Ticket DX = 3 crystals.
- **Crystal Shop.** The SKU number is the pack's total crystals:

  | Pack | Price | SKU |
  |---|---|---|
  | 1 | $0.99 | `credit.1` |
  | 5 + 1 free | $4.21 | `credit.6` |
  | 12 + 3 free | $10.10 | `credit.15` |
  | 20 + 6 free | $16.87 | `credit.26` |
  | 28 + 12 free | $23.56 | `credit.40` |
  | 42 + 26 free | $35.29 | `credit.68` |
  | 78 + 57 free | $65.64 | `credit.135` |

- **Monthly Courses** (subscription SKUs bronze, silver, gold):
  - Standard ($2.78): opens all memories.
  - Special: energy max up to 8.
  - Delux: regen 3 hours instead of 4.
- **Japanese edition:** the pre-registration bonus was 10 恋結晶 ("worth ¥1,000").

### UI (rebuilt from recordings)

`server/templates/index.html` was rebuilt on 2026-10-09 to match these reference frames:

| Screen | Source |
|---|---|
| Home, MENU, Character cards | YouTube `YYlJJHsm0t0` 1:00, 3:24, 3:30–4:03 |
| Memory, Memory Pass dialog, out-of-energy, Crystal Shop | `FNCwWtZM420` 2:30–3:51 |
| Post-story "EXP is now x%!" popup | `sXvwTBCjVR8` 1:12 |

- **Home:**
  - The heroine fills the screen; a house icon is top left.
  - **Get Crystal** is a gold hexagon, bottom left.
  - **Story ▸▸** is a pink ball, bottom right, with a name tag, the route %, and a gold bar
    with a heart.
  - The bottom bar has «portrait | **Menu** tile | portrait». While a window is open it shows
    **Crystal** n | Menu | **Energy** n/m instead.
- **The Story button plays the next unread part directly.** There is no list.
  - After a part, a popup titled "EXP is now x%!" shows the heroine and a speech box. In Aki's
    words: *The next story is "Chapter 4 - Wall of Obstinacy 4"!! I'm gonna die of curiosity,
    you know?* Aoi's line is "I wanna know what's gonna happen next...".
  - The popup also has the Memory Pass banner: "I don't wanna lose the memories with you… If
    you wanna save the story with her, CLICK HERE!"
- **Out of energy:**
  - "Uh-oh! It looks like you used up all your energy. Don't you wanna read more? You can
    fill up the energy bar, if you use Crystals."
  - Below that, **Fill up Energy Now!** with a ticket card ("Restore one energy bar with 4
    Bronze Tickets · You have: 14 -> 10"), red **Use** and blue **Close**.
- **Windows:** a cream panel with a gold border and an orange title bar with a house icon, plus
  a round red ✕.
- **MENU:** the Memory Pass banner, then a 4×3 grid of green tiles with white icons: News,
  Present, Photo, Item / Shop, Gacha, Dress Up, Character / Memory, More, Inquiries.
- **Memory:**
  - A heroine ribbon (‹ portrait + name ›, neighbours peeking in at the edges) and the tabs
    Main Story / Secret Story / Epilogue / Event Story.
  - One striped cyan row per part, e.g. "Chapter 18 - Child And Father 7".
  - Replayable rows are pink, underlined links with a blue ›. The others are plain; tapping one
    opens "Check Memory Pass to open up all memories you have got!" with [Check the detail
    about Memory Pass] [Go back to Memory].
- **Character:**
  - A spiral-notebook card per heroine with a colour-coded ribbon, height/weight and
    measurements, a quote, the blurb, and a round **Select This Girl!** button.
  - The real text for 5 heroines is in `heroines.json → card`.
  - A not-yet-unlocked heroine shows a grey "???" silhouette card.
- **Story box:** heroine lines use a pink box with a speech tail, and narration and the
  protagonist a cyan box without one. A heroine's (thoughts) use a yellow box with bubble dots.
  - The protagonist's name defaults to **William** and is editable.
  - Parts end on "To Be Continued"; a route ends on "Fin".
- **Home screen rebuilt to match the recordings (2026-10-10 evening)** — Thanh Bui `XvnBVVhYjh8`
  1:30 (1080p) and Ike's Yuri Stash `FNCwWtZM420` 3:36–3:48:
  - The share house behind her (`img/heroine/home_bg.png`, a Prologue frame of the house, gear
    painted out, upscaled 4×).
  - Her standing art (`img/heroine/<id>_stand.png`): the official site's character art
    (`famille.novel.mfapps.jp/img/sp-image-character-*.png`, Wayback) with the site's speech
    bubble removed — where the bubble covers her, the pixels come from the desktop site's copy
    of the same art (aligned with SIFT) or, where both copies are covered, by un-blending the
    bubble's ~73% white. Misaki's left arm is mirrored from her right (it is white-on-white under
    both bubbles). Then upscaled 4×.
  - **Mask repair (2026-10-12, `research/fix_stand_alpha.py`):** the cut-outs had small enclosed holes and
    half-transparent blocks in the mask, which the upscale turned into square specks of background, so the art
    looked pixelated. The script fills enclosed gaps under 3,000 px and repaints them (OpenCV inpaint), trims the
    1-px light fringe, softens the outline, and redoes the bottom fade as a smooth gradient. It also writes
    `<id>_face.png`, a square head crop used for the « » pills (`face_url`), which now show the whole face
    centred. The originals are kept in `img/heroine/_orig/`.
  - **Limits and Aki (2026-10-12):** the official site's art is a 256-colour palette PNG with the heroine only
    ~360 px wide, so the other five will always look soft at phone size. Aki's default art now comes from the
    English Google Play screenshot `gp_50` (640x1136, lossless, ~520 px wide; `research/art_src/store/`).
    `research/aki_stand_from_store.py` upscales it 2×, aligns it to the outfit shots'
    background, and cuts her out. Areas no screenshot shows empty are decided by colour (k-NN in Lab), plus one
    hand-marked siding block. It also removes the top bar's tint and trims its rim. `fix_stand_alpha.py` skips Aki.
- **Aoi's standing art from a store screenshot (2026-10-13, `research/aoi_stand_from_store.py`):** the Vietnamese
  store screenshot `mega_assets/Random/screen-2.png` (1440×2560) draws her home-screen art large and in full colour
  (smiling, mouth open), in front of promo cards. It's cut with `cut_anime.py` (2× up), cropped
  at mid-shirt so her head keeps the same share of the height. A polygon erases a promo card behind her right arm.
  It replaces the dithered site art (kept as `img/heroine/_orig/04_stand_site.png`), her pill face is redone, and
  `fix_stand_alpha.py` skips her too.
- **Outfits re-cut (2026-10-12, `research/costume_cutout2.py`):**
  - **More background known:** the background plate also uses `gp_50`, aligned on the house.
  - **Hair under the top bar kept:** the cyan bar is translucent, so hair under it is kept, with the bar's tint
    removed by a per-channel fit across its edge. This fixes the flat-topped heads (coat, swimsuit).
  - **Enclosed gaps:** gaps the figure encloses that match the known background are cleared, which removes the
    house wall and window left between the maid's ribbon and her hair. Leftover house strips beside the yukata
    are gone too.
  - Cyan ticker at the top (latest news title), gold "Get Crystal" hexagon with the "First
    Purchase SALE!" ribbon, the pink Story ball with a book behind "Story▸▸" and the % pill, the
    paper name tag, the gold progress pill with a heart lock, and cyan pills at the bottom with
    the neighbouring heroines' faces around a white Menu tile. Fonts: M PLUS Rounded 1c and Kalam
    (served from `static/fonts`).
  - Memory ribbon shows her face art; Character cards show her art large on the right.
  - Post-story popup: her portrait from the real popup (`img/heroine/<id>_popup.png`), upscaled.
  - Chapter title lines: her name card (`img/resource/bg/card_<id>.png`).
- **Item Shop** has the Memory Pass 30/90/180 (9/23/40 crystals) besides Gold Ticket (5) and
  Panel Ticket DX (3) (`FNCwWtZM420` 3:48). A pass opens every memory until it expires
  (`users.pass_until`).
- **Post-story lines (all six now recorded):** another part follows → *The next story is
  "…"* plus Ren "I'm so curious.", Aki "I'm gonna die of curiosity, you know?", Mitsuka "What's
  gonna happen next?", Aoi "I wanna know what's gonna happen next...", Sayaka "Aren't you curious
  to find out what happens if you continue?", Misaki "I wonder what'll happen next? I can't wait
  to see...". After a chapter's last part each has a chapter-end line (in `index.html`
  `END_LINE`).
- **Memory rows are grouped under chapter headings** with a chevron; the newest chapter is open
  (junior Chicoine `p99LNijyFWc` 19:20).
- **Secret Story tab:** an entry appears under its chapter as soon as that chapter is read
  ("Chapter 4 - Wall of Obstinacy" → "Secret Story 1: I Can't Be Docile", `p99LNijyFWc` 19:29),
  costs energy like a normal part, and ends on "To Be Continued". The list is
  `server/data/secret_stories.json`; only Aki's first one has been found.
- **Touching the heroine** (DevilGh0stz `_Ve8sVkG1XM`, 2017-12-01):
  - Plays a short scene in the story viewer: a load bar, 3–5 lines with their own picture
    (Aki at home in glasses, Sayaka in a café, Aoi in a café), no "To Be Continued". It
    costs no energy.
  - Then a popup titled "EXP 100.0%" (no "is now") with a line from her and the Memory Pass
    banner. Aki: "What? You've come to see me again? ...Thanks."; Sayaka: "I love you,
    {name}... Let's stay together forever."; Aoi: "{name}! Heh-hee hee... I just wanted to see
    what happened when I called you. Wait up!"
  - The scenes show her usual clothes even while a costume is worn, so they don't depend on
    the outfit. All three were at EXP 100%; whether the scene changes with EXP is unknown.
  - **In the mock:** `server/data/touch_scenes.json` (built by `research/touch_scenes.py`;
    pictures `img/resource/bg/Touch_<id>_<n>.png`), served as scenario `53<id>09010101`. The
    arc `09` is the mock's own; the real ID wasn't captured. The page returns to
    `#/my/touched/<id>` for the popup.
  - **Chat bubbles (all six):** touching usually shows a speech bubble over her instead
    (Japanese footage, 22 lines translated, `server/data/touch_lines.json`; section 4 "Fifth
    pass"). The mock shows the next line each touch; for Aki, Sayaka and Aoi every third touch
    plays the scene above.
- **Dress Up (gacha outfits):** the equipped outfit replaces her standing art on the home
  screen. The English edition's four with art (DevilGh0stz's 1080p screenshots,
  imgur `7CWVK`, 2017-11-23; video `_Ve8sVkG1XM`: "I can only unlock 4 for now", after saving
  up "jem" and "turning the capsule"):
  - Aki: winter coat (grey trench coat with a fur collar)
  - Sayaka: yukata (blue checked yukata under a brown haori)
  - Aoi: swimsuit (pink floral bikini)
  - Sayaka: maid outfit
  - **Ren: six outfits added later the same day** from the "Mega Assets" pack (see section 4, "Mega
    Assets pack"), cut out by `research/ren_outfits.py`:

    | Outfit | Source | Quality |
    |---|---|---|
    | White Blouse | 1080×1920 home screenshot from the Traditional Chinese edition | excellent |
    | Maid Cafe | event card ようこそファミーユカフェへ | good |
    | School Idol | event card ファミーユ☆スクールフェス | good |
    | Summer Yukata | event card カノジョとふたりですごす夏 | good |
    | Swimsuit | event card 海辺のシンデレラ | good |
    | Wedding Dress | event card 彼女がドレスに着替えたら | good |

    The cards are ~564×745 with Ren ~250 px wide, so each is upscaled 4× before cutting.
  - **The other heroines: 19 more outfits (2026-10-13)** from the same pack's cards, cut by
    `research/mega_outfits.py` (installed with `--install`). Every heroine now has outfits:

    | Heroine | Outfits added |
    |---|---|
    | Aki | Maid Cafe, School Idol, Police Uniform, Swimsuit, Wedding Dress (plus the English Winter Coat) |
    | Mitsuka | Maid Cafe, School Idol, Police Uniform, Summer Yukata, Wedding Dress |
    | Aoi | School Idol, Summer Yukata, Wedding Dress (plus the English Swimsuit) |
    | Sayaka | Police Uniform, Summer Yukata (rose pattern), Swimsuit (plus the English Yukata and Maid Outfit) |
    | Misaki | Maid Cafe, Summer Yukata, Wedding Dress |

    - "Police Uniform" is the new ファミーユ☆ポリス set; there's no Ren card for it.
    - Sayaka's maid card isn't used: the English 1080p screenshot is better.
    - Aki has two swimsuit cards; the one that draws her larger (`91be3b18`) is used.
    - **Hand fixes:**
      - `--keep` polygons remove the big background face's hair (Aki's maid and wedding cards) and the KEEP OUT tape
        (Sayaka's police card).
      - Maple leaves on the maid headdresses are erased, with holes inpainted. The headdress tops are a little ragged
        where leaves overlapped them.
      - Purple and cyan confetti on the wedding dresses is repainted.
      - A small sparkle stays beside Sayaka's police cut, and a faint yellow glow edge on her rose yukata.

  - **Aki: 10 more outfits, plus a better police cut (2026-10-13)** from about 15 home screenshots of the
    **Traditional Chinese edition** in `Outfits/Unsorted/` (~490×880). They show her real home-screen art per
    outfit: Mint Camisole, Cardigan Dress, Bathrobe, Coat and Scarf, Santa Dress, Fur Parka, Frilly Blouse,
    Pink Hoodie, Witch, Gothic Dress, and Police Uniform (replacing the event-card cut).
    - **Script:** `research/aki_home_outfits.py`, using the new `research/untint_bar.py`.
    - **Top bar:** the translucent top bar covers her crown and hats. Mirroring hair upward (Ren's blouse method)
      gave a flat-topped head here. Instead, `untint_bar.py` fits `under = k·clear + b` per channel across the
      bar's edge line, on the columns where she crosses it, and inverts it. This restores the crown and the witch
      and Santa hats; a slight blur stays along the old edge line.
    - **Bar leftovers:** the scrolling text and the house icon are painted out before matting.
    - **Roof between hair strands:** navy roof showing between the strands is cleared near her outline. On the
      witch and gothic shots, a pale window-frame stripe is erased along lines located by plotting the pale
      pixels. A dark patch behind her neck remains on those two.
    - **Skipped:** the grey coat (it's the English Winter Coat) and the mauve cardigan (her default clothes).
    Aki now has 16 outfits.
  - **Ren: Cat-Ear Maid (2026-10-13)** from a 720×1280 Chinese-edition home screenshot in `Random/`, by the same
    script. That shot has an opaque 「Famille Cafe」 event banner instead of the translucent bar, so the crop starts
    under it. Her blue-black hair is roof-coloured, so the roof and dark-text filters are off for her. Ren now has
    7 outfits.
  - **Aki: Navy Yukata (2026-10-13)**, her 17th outfit. Its only source is a Chinese store ad of the Dress Up
    screen (`Outfits/Kimono Set/2016110948547373.jpg`, her ~205 px wide); the pack's kimono cards cover everyone
    but her.
    - Cut by `aki_home_outfits.py` (PROMO), stopping at the obi.
    - A hand polygon follows her hair on the right, where the ad's sunset picture frame shows behind her. A narrow
      golden-colour rule clears the frame under her ponytail; her hair is too close in colour for a wider one.
    - A faint store watermark crosses her cheek, and a small orange scrap stays under the ponytail.
  - **Taiwan-edition wardrobe: 125 outfits (2026-10-14).** The biggest source by far: a Taiwan player's
    screenshots of their whole wardrobe on the server's last day (section 4, "Seventh pass"), ~24 home-screen shots
    per heroine at 1080×1920, one per outfit owned. Every heroine now has 23–26 outfits:

    | Heroine | Outfits | From the TW shots | Shots skipped |
    |---|---|---|---|
    | Ren | 26 | 21 (19 new) | default, Cat-Ear Maid (already in), a repeat |
    | Aki | 25 | 11 (8 new) | default, 11 already in from the Chinese-edition shots |
    | Mitsuka | 23 | 23 (18 new) | default |
    | Aoi | 23 | 23 (19 new) | default |
    | Sayaka | 25 | 25 (20 new) | default |
    | Misaki | 23 | 22 (20 new) | default |

    - **Scripts:** `research/tw_wardrobe.py` (`classify`, `fitbar`, `cut`) cuts every home shot into
      `research/hunt2/merge/tw/<id>_<nn>.png`; `research/tw_costumes.py` holds the picks and names, copies them to
      `img/heroine/costume/<id>_tw<nn>.png` and prints the `COSTUMES` entries.
    - **Top bar (fitbar):** the bar is the same overlay in every shot, so it's fitted once over all 144 as
      `under = K·clear + b[y, x]`. K comes from hair and background pixels just above and below the bar's edge
      (colour is continuous there): **0.346** in all three channels, so the bar is about 65% opaque. The offset b has
      a red tint that falls from left to right (106 → 77), and a row gradient taken as the mean row-to-row change
      under the bar across all shots (red rises ~26 levels toward the top). Hair under the bar now matches below it
      within ~2 levels. The first attempt (k fixed at 0.5, b per row fitted over sky) left dark hair ~30 levels too
      bright under a tinted band, and the matting then trimmed the band like hair; all 144 were re-cut.
    - **Cut:** the text and house icon are painted out, the bar undone, the shot cropped at 62% of its height
      (above the side buttons, so the cut-outs are busts to mid-chest like the home screen shows), then
      background removal (largest piece, bottom fade).
    - **Replaced:** 21 older cut-outs of the same outfit, mostly Mega Assets cards: the maid, idol, police,
      yukata, swimsuit and wedding cards where the TW shot exists, and Ren's White Blouse. The card files stay on
      disk (`<id>_maid.png` etc.); `COSTUMES` points at `tw<nn>` instead.
    - **Card versions back (2026-10-15):** the TW shots stop at mid-chest (the home screen's buttons hid the rest),
      while the cards show her to the waist or full length, so 20 of the replaced cards are offered again as
      "<outfit> (Card)", listed right after the outfit (`CARD_ART` in `app.py`; `ALL_COSTUMES` = everything wearable).
      They aren't gacha prizes: owning the outfit unlocks its card version (`POST /api/costume` accepts it then).
      No Face tab overlay on them (different head art). Ren's Chinese-edition `01_blouse.png` stays unused (same
      framing as the TW shot). Checked on the emulator with Sayaka's Maid Outfit (Card).
    - **Names** are descriptive (the TW item names aren't in the shots). Outfits the heroines share have the same
      name: Haori (brown haori over a blue check kimono), Ascot Blouse, New Year Kimono, School Blazer, Santa Dress,
      Bathrobe, Houndstooth Vest. Sayaka's card-named "Yukata" was the haori and is now "Haori".
    - **Judgement calls:** Aoi TW:27 and Mitsuka TW:28 (strapless white, only the top in shot) are taken to be the
      wedding dresses (a judgement call). Mitsuka has two maid outfits (Maid Cafe and Blue-Bow Maid).
    - **Leftovers:** a faint light line along the bar's old edge on dark hair; a few bumps on Aki's crown.
    - **Still missing** against the board's counts (Ren 30, Aki 29, Mitsuka 30, Aoi 29, Sayaka 31, Misaki 30):
      4–7 outfits per heroine, which this player didn't own.
  - **Dress Up > Face: 8 expressions per heroine (2026-10-14).** The board's 9 expression names (section 4, "Seventh
    pass") now have art, lifted from the story pictures.
    - **Why it works:** the story sprites share the TW wardrobe outfits' frontal head. A story picture's head matches
      a TW cut-out's head almost pixel for pixel at ~0.745× (normalised cross-correlation ~0.99), so the face a
      heroine wears in a story picture fits over any of her TW outfits.
    - **Script:** `research/face_items.py`.
      - `scan` template-matches each heroine's head (from a reference TW cut-out, `REF`) over all 9,880 story
        pictures, about 20 min per heroine at the CPU cap. Frontal heads found: Ren 407, Aki 971, Mitsuka 510,
        Aoi 989, Sayaka 943, Misaki 461.
      - Each face is cut to a per-heroine box (`FACE_BOX`, read off a grid drawn on a story face in reference
        coordinates; `faces/grid.py`); `recrop` redoes it without rescanning.
      - `sheet` groups similar faces into contact sheets (`research/faces/sheet_<id>_*.png`).
      - The faces that best fit each name are picked by eye (`PICKS`).
      - `build` colour-matches each face to the outfit's skin, feathers its edges and writes
        `img/heroine/face/<id>_<name>.png` plus `data/faces.json`.
      - `layout` finds where the face sits in every TW cut-out (`data/face_layout.json`).
    - **Names:** Smile, Angry, Crying, Shy, Surprised, Confused, Squint, Very Shy (笑顏 生氣 哭泣 害羞 驚訝 困惑 瞇眼
      超害羞); 初始 "Default" is the outfit's own face.
    - **Checked against the game's own icons (2026-10-14):** the TW wardrobe dump has each heroine's Face tab, with
      the 5 expressions that player owned (default, smile, angry, crying, shy; `research/faces/icons/`).
      `faces/icon_match.py` scored every story face against them, and 14 of the 24 smile/crying/shy picks were
      replaced by the story face that is visibly the same layer (`ICON_CONFIRMED`; `faces/icon_vs_story.png`).
      Angry (a sideways glare) isn't in any frontal story face, and surprised, confused, squint and very shy have no
      icon, so those stay my judgement. Laying the 272 px icons themselves over the outfits was tried and dropped
      (frame lines and upscale ghosting).
    - **In the mock:** the Face tab lists them (table `dress_face`, `POST /api/dress_face/<id>`), and the home screen
      lays the chosen face over the outfit (`face_overlay` in the player state). It works only on the 125 TW outfits;
      the default stands and the card cut-outs use other art, so they keep their own face (the tab says so).
    - **Fixed 2026-10-15 — doubled portrait:** the home screen drew the outfit twice, as `#heroine`'s own background
      (full height) and as a `.pic` child sized "contain". On outfits wider than the screen (the bust-up TW ones)
      the two copies didn't line up and showed as overlapping portraits. `renderHome` now adds the `.pic` only when
      there is no art; checked on the emulator (Mitsuka's Green Cardigan, Sayaka's Maid Outfit).
    - **Limits:** a faint hair ghost at the edge of a few faces (Aki's Smile), and hats or hair ornaments that reach
      into the face box would be covered (none seen).
  - **Dress Up > Background: 8 more from the board's list (2026-10-14).** The Taiwan board names 13 backgrounds
    (outside the house morning/evening, each heroine's room, the living room, the balcony morning/evening/night, the
    school cafeteria). They're story locations, so `research/location_bgs.py` groups the 9,880 story pictures by their
    outer quarters (which characters rarely cover; 351 groups), names the groups from the story text shown over them
    ("living room" 109×, "Aki's room", "Aoi's room", "Ren's room", "balcony", "cafeteria" 8×) and takes the member
    with the least face-skin colour in the middle as the empty view (`research/locations/`).
    - **Added** (`dress_backgrounds.py`, picked by eye): Famille House (Evening) `Aki_C04_002`, Living Room
      `Aki_C02_013`, Aki's Room `Aki_C20_003`, Aoi's Room `Aoi_C14_011`, Ren's Room `Ren_C11_002`, Balcony (Evening)
      `Aki_C04_031`, Balcony (Night) `Aoi_C10_001` (the same wooden balcony), School Cafeteria `Prologue_01_006`. The
      morning exterior is the house itself (the default).
    - **Added 2026-10-15:** Misaki's room `Misaki_C02_001` (also `C25_002`, `C51_001`; the group's "cleanest" pick
      had Mitsuka in front, so it was missed) and Mitsuka's room `Aoi_C04_052` (Aoi Chapter 4, William brings Mitsuka
      apple tea; the same pink bed and drawers as Mitsuka C47–49 at "Mitsuka's door"; the recording's 26-row black bar
      cut off). Both had empty views, so nothing had to be painted out. Their corners keep the original pixels (the
      gear-button refill would have copied a chair or a table leg).
    - **Locked ("?"):** Sayaka's room never appears in the story ("Sayaka's room" is in no line; her route's room
      scenes are William's room, a hospital or a hotel), and the balcony only at evening and night (searched by layout
      over all 9,880 pictures, 2026-10-15). With no picture of either, there is nothing to paint a heroine out of.
    - The store ad's items (garden, bridge, cherry blossoms, beach) aren't on the board's list; both are kept, so the
      tab has 17 slots, 13 with art.
  - **Dress Up > Background (2026-10-13).** The same ad shows the Background tab: the house, a vegetable garden,
    a bridge at sunset, cherry blossoms, a beach, then "?" slots.
    - `research/find_backgrounds.py` compares the ad's tiles with 24×24 thumbnails of all 9,880 story pictures
      (cached in `ocr/bg_thumbs.npz`), plus a colour-content search. It found the **garden** (`Mitsuka_C04_002`)
      and the **beach** (`Misaki_C31_031`), both with nobody in front.
    - The bridge and the cherry blossoms appear in no recorded story, so they show as "?".
    - `research/dress_backgrounds.py` patches the smeared bottom-left corner left by painting out the recording's
      gear button, writing `img/heroine/bg/{garden,beach}.png`.
    - **In the mock:** the Background tab lists them; the choice is kept per heroine (table `dress_bg`,
      `POST /api/dress_bg/<id>`) and drawn behind her on the home screen. How backgrounds were obtained in the
      game isn't known, so all recovered ones are available.
    - **Face items:** the same ad shows four of Aki's expression tiles (~95 px), too small to use. The real
      expression overlays are lost.
  - **Character card text confirmed:** the *Honey I'm Home!* "Choose Your Girl" screenshots in `Random/` carry the
    same English bios, word for word, for Ren, Aki, Mitsuka, Aoi and Sayaka (noted in `heroines.json` as
    `card._confirmed`). Heights and weights agree, and the age and year are given as "College Sophomore" and so on.
    Aki's quote there is "…you brute!"; My Sweet Roomies used "This is just for the sake of my role!". There's no
    Misaki in that game, so her card text is still missing.

    The note below describes the state before the pack.
  - **No outfit for Ren, Mitsuka or Misaki** (checked 2026-10-12, before the pack). Ren is only ever seen in other
    clothes in promotional art, too small to use:
    - On the Japanese gacha banner (`img/ui/gacha_banner.png`), in a black-and-white gothic dress with
      hair ribbons. She's partly covered by the 「アイテムGet!!」 lettering, and the banner is a 4×
      upscale of a blurry video frame.
    - In the gacha tutorial pictures (`FVh_ajT6wcM` 27:34, `WDFQmxmtR38` 7:05, English `EqwSQBUXVfE`),
      in cat ears with a maid ribbon or a tiara, a few dozen pixels per heroine.
    - No recording shows a player wearing an outfit on her. The source hunt's scan of 358 English
      uploads found no worn outfit at all, and DevilGh0stz's four screenshots are the only ones known.
    - A cut-out from the banner would look far worse than her default art, so the mock offers none.
    - **Lead:** Yu_Tsugami (`UCfwim5bqEcAg0iXTrt-wd9Q`) and SandDistress 83 (`UCFimGR1O92kM955qhxLmQ0w`)
      play Ren's whole route and their home screens haven't been checked frame by frame. A scan would
      take a couple of hours at the CPU cap.
  - `research/costume_cutout.py` cuts them out into `img/heroine/costume/<id>_<name>.png`.
    The four screenshots share the same background, so a pixel is background where it
    matches a different heroine's screenshot and connects to the edge. GrabCut cleans the edge,
    and one patch of roof behind Aoi (same blue as her hair) is masked by hand.
  - The mock's gacha draws only from a heroine's recovered outfits. Since 2026-10-13 every
    heroine has some (a heroine with none would say so instead of pulling). The in-game outfit names weren't
    recorded, so the mock's names are descriptive. The earlier guessed list (Glasses, Pajamas,
    …) is gone: the "glasses" were Aki's look in her touch scene.
- **Gacha window:** the real 「恋結晶でガチャ 着せ替えアイテムGet!!」 banner (`img/ui/gacha_banner.png`),
  gacha points and one pull for 5 crystals (or "You need N more crystals… Go to the Shop!").
  - **Pull flow and animation** (added 2026-10-12): confirm "Use 5 Crystals to spin the Gacha?
    [Spin!] [Not now!]" → the capsule-machine animation with a Skip button → the gacha-get jingle
    (`se-system-gachaget.ogg` from the APK) → a result card "[Outfit] Sayaka's Yukata GET!!" with
    the outfit's art, then Spin again / Dress Up.
  - Famille's own animation was never recorded, so it comes from sister game 恋Q部 (same Smart
    Novel engine and gacha UI; ハルク `EygEZekAZhU` 2:10–2:15): the 420 px square cut from the
    recording, upscaled 4×, 30 fps → `img/ui/gacha_spin.mp4` (5 s). Its confirm wording
    「恋結晶5個を使ってガチャを引きますか？ [ガチャる!] [今は辞める!]」 and result layout 「[服装]芹香の寝巻き
    をGET!!」 are the models for the mock's text.
- **Dress Up window:** tabs Expression / Outfit / Background, a grid with "?" for items not owned
  and the worn one highlighted.
- **Photo window:** tabs Photo Panel / Event / Epilogue, the 4×4 panel board over the current
  picture, and a viewer with captions (section 4 "Footage hunt").
- **Maintenance screen:** "Under Maintenance – Please check back later. [Close the app.]"
- **News:** the five real 2019 posts (`server/data/news.json`), including the full closure
  notice.

## 4. Story text

Status as of 2026-10-12: **story OCR complete for every route.** Every main-story, epilogue and
Prologue part has recovered text; only 55 secret stories (no English source) play the placeholder.
On 2026-10-12 Aoi Chapter 27 (5 parts) and Aki Chapter 36 (2 parts) were added (`Aoi_C27`,
`Aki_C36`). Later that day the skipped-line pass ran (below), and it also found a missing
upload (Sayaka Ch.48 part 2) and two crash-split chapters. The table is the state after that.

| Route | Chapter videos done | Compiled story parts | Lines | Quality |
|---|---|---|---|---|
| Prologue | — (transcribed by eye) | 2 | 160 | exact |
| Aoi | 57 / 57 | 240 | 5,967 | good |
| Aki | 59 / 59 | 252 | 6,449 | good |
| Mitsuka | 54 / 54 | 229 | 5,557 | good (sample: ~99.5% characters) |
| Misaki | 56 / 56 | 248 | 6,403 | good |
| Sayaka | 56 / 56 (+ `Sayaka_C48b`) | 308 | 7,189 | good |
| Ren | 55 / 55 | 231 | 5,656 | good |

Total: 37,381 lines.

**Skipped-line pass (2026-10-12, done):**
- `find_gaps.py` flagged 593 long gaps. Instead of checking them by eye, `research/gap_rescan.py`
  downloads only the gap windows (`yt-dlp --download-sections`, ±1.5 s), decodes them at 6 fps and
  OCRs every new box content without waiting for the text to settle. It keeps text that doesn't
  match the transcript around the gap (short lines like "No... no!!!" recur, so it doesn't compare
  them against the whole chapter). Output: `ocr/gaps/<video>.json`. It took ~1 hour on 8 cores.
- 233 candidates → 97 after dropping junk and anything that matches a line elsewhere in the chapter
  → **67 lines accepted** after review, by `research/build_gap_patches.py`. They're appended to
  `patches.json` (68 entries in all), each anchored to the line before its gap, with the OCR reading
  kept in `source`. Examples: Aoi Ch.26 `(No... no!!!)`, Aki Ch.29 Mitsuka's "Recently your color
  hasn't been very good…", Mitsuka Ch.35 the High School Boy's "group of angels" line. Rejected:
  misread quote marks (`11`, `iii`), half-read fragments (`know...`, `-Doctor`).
- The rest of the gaps are long reads ("… …", pauses), not lost text.
- **Missing upload found:** `chapter_videos.txt` listed two uploads as `Sayaka_C48`: "Chapter 48
  part 1" (`AmR3GhGV2Zk`) and "part 2" (`DtkfO2cdCYQ`, 416 s). The queue skips names that are
  already done, so part 2 was never read. It's now `Sayaka_C48b` (119 lines). Its part 1 starts
  mid-scene ("No! There's no need to apologize." after "(P-Perhaps the nurse is mad...?) / When..."),
  so `compile_story.py` (`CONTINUATION`) appends it to part 5. **Sayaka Ch.48 has 9 parts** (was 5).
- **Crash-split chapters fixed** (checked from frames). The part counts came from loading screens,
  and an app restart adds one:
  - **Aki Ch.4:** after part 3, a connection error → title screen → Memory list → "Wall of
    Obstinacy 4", then the popup "The next story is … 5". The menus had become parts 4 and 5 and the
    real parts were numbered 6 and 7. Now **5 parts** (was 6); `PART_MAP` and `DROP` in
    `compile_story.py`.
  - **Misaki Epilogue 2:** the app crashed ("has stopped") right after "Heh... well then, cheers!".
    The uploader reopened the epilogue and skipped through it to that point (763–896 s), then went
    on. The replay is dropped (every line in it is already in part 1), and the rest joins part 1:
    **1 part** (was 2; it had compiled as 4, with junk "Start the Game!" / "Misaki Story" parts).
  - A search of all OCR outputs for menu text ("Start the Game", "<Name> Story") found no others.
  - `chapter_parts.json` `_corrections` records all three.

**What was fixed on 2026-10-10:**
- **Ren C17–C53 (35 chapters) re-run.** These are full-screen 720x1280 recordings whose box is
  ~0.47 of the width tall; `box_span()` now accepts 0.30–0.50. Ren went from 2,462 to 5,647 lines.
  Checked in the real app: Ren Chapter 20 part 1 matches the source video line by line.
- **Sayaka C14, C16, C17 finished** (the overnight run had stopped before them).
- **Night-scene videos (Aoi C32, Aoi C33, Misaki C29).** The game-area detector uses a brightness
  cut-off. On mostly dark videos it shrank the area (Aoi C32: 355 px wide, starting at row 329), so
  the box search started below the box and returned 0–10 lines. When no box is found,
  `story_ocr.py` now retries `game_area(sample, dark=5)`. Chapters that already worked are
  unaffected, because the retry only runs when the first pass fails.
- **Two part counts corrected** in `server/data/chapter_parts.json`: Aki Ch.55 = 4 (was 5) and
  Misaki Ch.23 = 3 (was 4). Both uploads end with "To Be Continued" and only a "Back to List"
  button, and the popup before Aki's last part says "Melting In Her Tears 4". Recorded in the
  file's `_corrections` key.
- **`Sayaka_F99` is skipped on purpose:** it re-uploads the last minute of Sayaka C52.
- **Sanity check used:** lines per minute of video for every chapter (median ~22). Anything under
  half the median, or under 30 lines, gets looked at. After the fixes, the only chapters flagged
  are short videos (Ren C28 is 108 s long).

The broken outputs are kept in `ocr/out_v1/` as `<name>_tallfail.json`.

Earlier first-pass note: That's 15,829 lines so far. **42 chapters failed to download** in the first pass, all from
YouTube rate-limiting; none were OCR errors. They're listed in
`research/ocr/failed_first_pass.txt`. They aren't lost: re-running the queue picks them up,
because it skips everything already in `ocr/out/`. Downloads now retry automatically (after
90 seconds, then 5 minutes).

Transcripts are written to `server/data/scenarios/<story id>/transcript.json`. Each file's
`_source` field says where its text came from.

### How the text is collected
- **Prologue:** read by eye from a 2019 recording (`research/lines.py` sheets) into
  `research/prologue.txt`. One line may be missing around 5:56–6:05.
- **Routes:** OCR of DevilGh0stz's chapter uploads (`research/cards/list.txt` maps each
  chapter to its video). For each video, `research/story_ocr.py`:
  1. Downloads the video and finds the game area. It samples frames across the whole video,
     because a dark opening hides the top edge.
  2. Finds the dialogue box by its thin blue or pink border lines: two lines about 0.38 of
     the game width apart. Uploads were recorded on phones with different aspect ratios, and
     the box sits at a different height in each. The first version cropped at fixed fractions,
     which cut off the name and the second text row on other aspects. In Ren Chapters 15–53
     that merged the name into the text and lost half of each box. The 77 chapters with
     non-standard geometry were redone on 2026-10-09; the old results are in `ocr/out_v1/`.
  3. Keeps the frames where the dialogue box is visible and its text has stopped typing.
  4. OCRs the box with RapidOCR, grouping boxes into rows, then splits the speaker name from the
     text.
  5. Splits into parts at the loading screens. The result is `research/ocr/out/<Heroine>_<Cnn|Enn>.json`.
- **Compiling:** `research/compile_story.py <Heroine>` turns that into story files:
  - **Mapping:** maps uploads to chapters, including Aoi's skipped or duplicate numbers and
    Aki's "Final Chapter" = 57.
  - **Fixes the OCR's habits:**
    - standalone capital I read as `l`/`|`; `lf`/`lt`/`ln`/`ls` for If/It/In/Is
    - opening quotes read as `'`; closing quotes read as `'` or `11`
    - stray single letters from the typing cursor
    - a lost first or last mark: `SLAP!*` → `*SLAP!*`, `Ahhhh!!!"` → `"Ahhhh!!!"`, an
      unclosed `"…` gets its closing quote
    - a thought that lost its opening `(I`: `wonder if…)` → `(I wonder if…)`, `D) can't…)` →
      `(I can't…)`. This fixed 223 lines, and the app now shows them as thoughts, since it picks
      the box style from the leading `(`.
    - the ellipsis read as two dots (`today..` → `today...`); a closing ♪ read as `J`
  - **Cleanup:** drops half-typed duplicate lines and title-card lines, and snaps speaker-name
    typos to the cast.
  - **Player name:** each uploader's name (e.g. "Mihile") becomes the default **William**.
- **Hand fixes:** `research/patches.json` holds lines the OCR skipped, each with its source.
  They're re-applied on every compile.
  - **Example:** Misaki's line in Aoi Chapter 1, read from a frame at 0:18.
- **Skipped-line finder:** `research/find_gaps.py <Heroine>` lists unusually long gaps between
  OCR'd lines into `gaps.txt`; `gap_rescan.py` re-reads them (see "Skipped-line pass" above).
- **Proofreading pass (2026-10-15):** `research/typo_fixes.json` holds 1,046 fixes (story id, line index, old word,
  new word), which `compile_story.py` applies after the patches; all six routes recompiled.
  - **How they were found:** `proofread.py scan` lists words that are neither in an English word list
    (`pyspellchecker`) nor common in the story; `proofread_propose.py` tries the OCR's usual damage in reverse
    (`proofread_proposals.tsv` → `pp.tsv` / `pp2.tsv`). Three more lists: `pp3.tsv`, a stray letter that makes a
    real word ("sare" → "are"); `pp4.tsv`, a stray "a" or "I" between words and doubled words ("been a able to",
    "a I little", "I I work"); `pp5.tsv`, one wrong letter ("cogether", "wouid", and W read as N in Misaki's route:
    "Nhile", "Nalk").
    `proofread_choose.py` picks a default fix per row, and `proofread_overrides.py` holds the decisions made by eye
    (row number → fix, or `None` to keep the word). `python proofread_choose.py write` rebuilds `typo_fixes.json`.
  - **What was fixed:** a stray cursor letter glued to a word ("gto" → "to", "nmy" → "my", "sin front of" → "in
    front of", "eme" → "me"); a stray letter at the end ("truea" → "true"); lost spaces ("inmy", "Apparentlyshewas",
    "WhenIbow"); a closing ♪ read as J in Sayaka's lines ("datej" → "date♪", 38); a dropped letter
    ("pefect" → "perfect", "momen" → "moment"). Plus one more rule in `clean_text`: a lost opening asterisk
    ("Yawn!*..." → "*Yawn!*...", "Sigh*."), and another for a line break read as "/" ("show /off", "really / cute",
    93 lines). About 1,100 lines changed in all.
  - **Left alone on purpose:** interjections and sound effects ("Uuh", "Gahh", "*Tup tup tup!*"); British spellings
    ("behaviour", "realise", which are in the official text); misspellings the OCR wouldn't make, so probably
    official ("annoucned", "heared"); garbled lines read from the edge of the box ("Why.Zil +,upinom").
  - **Not a typo:** about 120 of Aki's thoughts end in "!?" without a closing bracket. A frame of Aki Chapter 10
    (`hwEC22HKwpQ`, 0:46) shows the game's own text is "(I've never seen her be so nice!?", so they were kept.
  - **Mitsuka's route** (second round): the same detectors in one list (`proofread_mitsuka.py` → `pp_mitsuka.tsv`,
    rows 50001+ in the overrides), 188 fixes by eye. Only OCR damage was corrected; nothing was added to her text.
- **Frame check (2026-10-15):** the OCR's worst habit is invisible to word lists: it **drops a short first word**,
  mostly "I" ("stand up to leave", "guess so"), sometimes a whole second row of the box ("... and now she's starring
  in another series - she really is popular!"), and reads a box's last word **upside down** ("tastes.ipoob" =
  "tastes good!", "What.zop I pinous" = "What should I do?", "Zil joN" = "No! I!?").
  - **Finding them:** `grammar_scan.py` (LanguageTool 6.8, local Java server pinned to 8 cores; 37,221 lines in
    20 min → `grammar_matches.jsonl`); lines starting lowercase; LanguageTool spelling and grammar flags; junk
    tokens. 2,392 lines in 333 uploads (`fc_list.tsv`).
  - **Checking them:** `compile_story.py` now writes `line_src.json` (upload and second for every compiled line).
    `frame_check.py` re-downloads each upload once, crops the dialogue box at that second (`frame_check/c_NNNNN.png`,
    kept as evidence; `rows.tsv` = row, line, upload, second, text), and deletes the video. `fc_sheets.py a b`
    draws review sheets; the readings went into `fc_decisions.tsv` (row → line as the frame shows it, player name
    → William), and `fc_apply.py` turns them into `line_fixes.json`, applied last by `compile_story.py` (only
    while the line still reads exactly "old"). **1,894 lines corrected from the frames** (~80% of the flagged ones).
  - **Other rules found on the way** (`after_fixes` / `WORD_MAP` in `compile_story.py`): "sme/smy/eyou/lsn't/
    ldol/awas/wasa/Wel" and similar; a closing "up" read as "dn" ("freezes dn" → "freezes up"); `rot_decode.py`
    decoded 36 upside-down endings into `line_fixes.json`.
  - **Totals:** 3,258 lines differ from before the proofreading pass (all six routes). Uploads `Aki_C55` and
    `Mitsuka_C08` failed once with HTTP 403 and worked on retry.
- **Full second read (2026-10-15):** every main-story line (all 336 uploads, ~36,700 lines) was read a second time
  from its own frame, to catch the errors that still read as plausible English ("Her face is burning red as Mitsuka
  takes 5 my hand", "I stand frozen I to the spot", "she's already turned about to explode").
  - **How:** `full_check.py` (2 workers via `FC_WORKER`/`FC_WORKERS`, 4 OCR threads each on 8 pinned cores, a
    40% CPU cap; resumable per upload, each video deleted once done) downloads every upload again, cuts the
    dialogue box at the second each line was read (`line_src.json`) at **960 px** (the first pass used 480-px
    frames, the source of most misreadings), reads it with RapidOCR and compares letters only, allowing for the
    watermarks ("MY SWEET ROOMIES", "Made With VideoShow"), the speaker row and the second read's own habits (a capital
    I read as l/T, a doubled letter at a word edge, ♪ as J, closing quotes as "11"). Lines that differ go to
    `full_check/diff_<w>.tsv` with their crop (`c_<sid>_<line>.png`; the crops and review sheets, 3.4 GB, were deleted
    2026-10-15 once the review was done, so `fc2_pick.py` needs `full_check.py` re-run for the uploads concerned);
    every checked line goes to `done_<w>.tsv`. The text files (diffs, `review.tsv`, logs, fix backups) are kept.
  - **Review:** `fc2_sheets.py a b` collects new differences into `full_check/review.tsv` (2,896 rows) and lists the
    ones still differing; `fc2_pick.py n…` draws the crops of rows the two texts can't settle. Readings went into
    `fc2_decisions.tsv` (1,416 rows; player name → William) and `fc2_apply.py` turned them into `line_fixes.json`
    entries. **1,414 lines corrected**, plus 22 in `fc2_extra.tsv` (below). Mitsuka's route got OCR fixes only.
  - **What the second read found:** short words dropped by the first pass ("a", "so", "I", "up"), "1" for "I",
    "groom" for "room", "ewe" for "we", "ours/hers" for "our/her", stray letters ("takes 5 my", "frozen I to"), lost
    ♪, cut-off endings of long lines, upside-down reads, and in Ren's route many short lines whose leading pause
    ("... Yeah.", "... ... Um...") came out as "11" or vanished. A first "11" isn't always "...": in other uploads it
    is a spaced opening quote, so each was checked on its crop.
  - **Quotation marks (`fc2_quotes.py`):** uploads that space their quotes out (`" text "`, mostly late Misaki and
    Sayaka chapters, some in every route) lost them in both passes, so ~2,000 spoken lines were bare. A sample of 44
    such crops were all in quotes (William's too), so every bare line under a name box now gets quotes, or brackets
    where the second read saw one; a line carried on from the box before keeps no opening quote. It also adds 44
    missing closing quotes and applies `fc2_extra.tsv` (lines the letter-only compare can't see, e.g. an upside-down
    "!?" stored as ".i..."). **2,013 lines given quotes.** Run it after `fc2_apply.py` (which rewrites its rows
    without the quotes), then recompile; it is safe to re-run.
  - **Left as the game wrote them:** Aki's `"!"Oh...` lines (the game's own stray quote, seen on the crops),
    "My eyes eat Misaki's", "I decide to say how what I really feel".
  - **Verified:** after the last apply and recompile every decision is in the transcripts (exact, or with the
    added quotes) and the 32 tests pass. The fix files: `line_fixes.json` 5,354 entries (1,893 from the frame check,
    1,435 from the second read, 2,013 quotes, 44 closing quotes, 27 upside-down endings from `rot_decode.py`); backups before the quotes and before the
    first apply are in `full_check/line_fixes.before_quotes.json` and `line_fixes.before.json`.

### Story pictures (added 2026-10-10)

The engine draws `scene.bg` behind the dialogue box: it downloads `img/resource/bg/<bg>.png`
(listed in the part's resource list) and scales it to the screen width, bottom-aligned to the
box. **It only accepts PNG:** a `.jpg` name is ignored and JPEG data under a `.png` name crashes
the app (both tried on the emulator). No original art survived, so `research/scene_frames.py`
takes, for every OCR'd line, the frame it was read from:

1. Crops the square scene area above the dialogue box (characters included). Uploads that were
   stretched vertically (Ren C17–C53) are un-stretched using the box's height.
2. Paints out the recording's gear button and bubble tail; the app draws its own on top.
3. Reuses an earlier picture of the same video when nothing changed. The comparison uses the
   most-changed 8×8 block, so a blink or a new expression still gets its own picture.
4. Decodes at the video's own resolution, then upscales each picture 4× on the GPU and saves
   a 1080×1080 full-colour PNG (~500 KB; the phone's screen width). The first version saved
   540-px 256-colour PNGs straight from the video, which looked blurry on the phone.
5. Writes `ocr/scenes/<video>.json`, one picture name per line. `compile_story.py` puts it into
   each line's `scene.bg`, and the server lists a part's pictures in its resource list.
6. Never crops above the game area (some uploads include the recording phone's status bar).

Run it with `bash ocr/scene_queue.sh <Heroine…>` (same download and CPU cap as the OCR queue;
resumable). The Prologue's lines were matched by text to Visual Novel Archive's recording
(`SscrK87yqlg`), and the secret story to HDBarbecho's (`AphjRVlp5gY`), whose screen recorder
logo and badge were blurred out with ffmpeg `delogo`.

**Limits:** pictures are video frames (480–720 px wide, compressed), not the original art.
Sprite changes and effects are baked in, and a bit of the painted-out corner can show at the
bottom left.

### Known limits
- **Typos:** expect occasional OCR typos and an occasional skipped line, where the player tapped
  through. The proofreading pass and frame check (2026-10-15, above) corrected 3,258 lines, and the full second
  read another 1,414 plus quotation marks on 2,013. What remains is mostly punctuation the letter-only compare
  can't see: a leading "..." the first pass dropped where the second read dropped it too, the odd ♪ or "!"/"?".
- **Aoi Chapter 54:** the text is complete (it continues straight from the Chapter 53 video), but
  the title card fell between uploads, so its title is unknown.
- **Measured accuracy:** checked against video frames on 2026-10-09.
  - **In the app:** 10 random lines of Mitsuka Chapter 1 came out 8 exact and 2 off by one
    token ("today.." and "out ton the"); the rule above now fixes the first. The passage around
    "\*SLAP!\*" matched exactly after the new rules.
  - **Character level:** about 99.5%. Speakers were correct in every sampled line.
- **Words the OCR drops:** a short word at the end of the first row is sometimes missing (e.g.
  "only **a** dream"). The full second read (2026-10-15) found and fixed these wherever its 960-px read saw the word.
- **Placeholders:** (none needed as of 2026-10-10) parts with no recovered text play a placeholder (the heroine's card quote,
  then "(This part's text hasn't been recovered yet.)").

### Footage hunt for the unknowns (2026-10-11 – 10-12, done)

Videos are in `research/ocr/hunt/` (24 files, 1.1 GB, 720p; list in `ids.txt`).
`research/ocr/hunt/nonstory.py <video> [step]` writes contact sheets to `hunt/sheets2/`.
It keeps only the frames without the cyan dialogue box (menus, gacha, popups), each stamped
with its time. It auto-crops portrait recordings. Its crop detection fails on 神崎LEN's
videos (pink frame) and on jZA73RCZEsk (a phone filmed by a camera).

**Japanese-edition footage (ようこそ！ファミーユへ; same engine, Japanese text)**
- 神崎LEN plays all six routes with full playthroughs, Misaki last (2019).
  Playlist: `PLppJyonqvsbjbmyMntn6jpkhGSh1pRNlj`. Misaki #1 is `ns9u-4bhHGU`; #2–#6 are
  `920RbGddG3g`, `uxrdNXkI-mU`, `I1jPBMh2CqA`, `tJTshlUQW5E`.
- ハルク Part 1–9 (`FVh_ajT6wcM`, `C97Hz0KiMO4`, `QjyIYJWmtKo`, `kIfzJSNbz6Q`, `U5W9Jc2MCpo`,
  `oIev7_0QsZA`, `uclp3rA790E`, `oqNvshACFh8`, `ITAfjWb14rg`) were recorded in the last month
  before the Japanese shutdown. They cover the tutorial, login bonus, Present box, Items and
  MENU. They never open the Gacha.
- Others: 恋愛倉庫 (Aoi route, short parts), けっつん `Gv8R3io6B-s`, iOS&Android Game Play
  `WDFQmxmtR38`, Dizzy Ziddy (Japanese with English subtitles).
- `335Hf6yAjwM` and `NSeD2vgXfvw` are spam ("free gacha" scams). Ignore them.

**What was found**
- **Gacha gives dress-up costumes.** The tutorial page 「ガチャで着せ替え！」
  (`FVh_ajT6wcM` 27:34, `WDFQmxmtR38` 7:02) reads: spin the gacha to get dress-up items,
  many items, dress any heroine freely.
  - Costumes shown: Misaki in a red coat, Mitsuka in a white dress, Aoi in a light dress,
    Ren in cat ears with a maid ribbon, Sayaka as a nurse.
  - appget.com's review: 「期間限定ガチャを回すことで特別ストーリーを見ることができるイベント」.
    So time-limited gachas unlocked special stories, which is probably where Event Stories
    came from.
  - The MENU banner 「水着の着せ替え大幅アップ中!!」 (`WDFQmxmtR38` 8:46) shows a swimsuit
    rate-up.
  - Gacha price and the full costume pool are still unknown.
- **Crystal shop (Japanese, 24-hour 80%-off sale page, `WDFQmxmtR38` 7:08).** Re-read on
  2026-10-11 from a sharp frame; the first reading had 30 and 136. The pack sizes are exactly
  the English SKUs (`credit.6` … `credit.135`). Every row is 「初回限定!」, valid only "within
  24 hours of starting the game" and "only on your first purchase in the shop":

  | Crystals | Sale | Normally |
  |---|---|---|
  | 6 | ¥100 | ¥500 |
  | 15 | ¥600 | ¥1,200 |
  | 26 | ¥1,000 | ¥2,000 |
  | 40 | ¥1,500 | ¥2,800 |
  | 68 | ¥2,500 | ¥4,200 |
  | 135 | ¥4,900 | ¥7,800 |

- **Tutorial page 「この他にも…」 (`WDFQmxmtR38` 7:03):**
  - **Login bonus, tickets:** "毎日連続ログインで… チケット上限が最大8枚!!" Consecutive daily
    logins raise the energy cap, up to 8. That means 8 is not only the Special course's cap.
    The mock doesn't do this yet.
  - **Login bonus, pictures:** consecutive logins give limited pictures (限定イラスト), and
    login-day totals give "deluxe items" (豪華アイテム).
  - **Monthly courses:** three courses, "pick one to suit how you play".
- **Gacha tutorial picture (`WDFQmxmtR38` 7:05, sharp):** the six heroines side by side in
  outfits: Misaki in a red coat, Aki in a pink blouse, Aoi in a light-blue dress, Mitsuka in a
  black-and-white dress, Ren in a tiara with cat ears, and Sayaka as a nurse. The capsule
  picture also shows picture cards (cherry blossoms, a beach), so the gacha may also have
  given pictures. None of these outfits have been seen in the English edition yet.
- **There was also a separate "online version"** (DevilGh0stz, "Aoi (Online version)" Ch 1–5,
  2017-11/12, `Z96fOqMFaPA` …). He wrote: Japanese menus, a different "clicking style" from
  the mobile app, a "slightly different" and longer story, and a poor English translation.
  This is probably a browser/PC build. It hasn't been looked at.
- **Japanese shutdown notice** (`C97Hz0KiMO4` 1:52–2:34):
  - Crystal sales stopped 2017-06-28 12:00.
  - Dates 2017-09-01 12:00 and 2017-11-15 12:00 appear. My reading: service ended
    2017-09-01, and refund requests closed 2017-11-15.
- **The "next part" popup** sometimes shows 「次回限定イラスト発生!!」 (a limited picture in
  the next part; `QjyIYJWmtKo` 8:20).
  - Popups in 2016 carried a 「ステーションメモリーズ」 banner (Station Memories!; `Gv8R3io6B-s`
    1:26), probably a cross-promotion or collaboration.
- **After a route ends,** the bar restarts at 0% and the story continues after the couple
  start dating, i.e. the Epilogue. Source: けっつん's description.
- **Misaki opens after the other five routes are finished (settled 2026-10-11).**
  - DevilGh0stz wrote under his Aoi final (`mIkmTpGmXxM`, 2016-01-11), after saying he had
    "already finished" Ren and Sayaka: *"But damn it, still got Aki to go in order to unlock
    Misaki..."*
  - His upload order wasn't his play order, which is why the upload dates had seemed to
    contradict the rule. He uploaded Mitsuka's final on 2016-02-20.
  - The first-launch picker shows her as a grey 「？？？？」 card with no condition text
    (`FVh_ajT6wcM` 24:44).
  - The mock now opens her when the other five main stories are at 100%.
  - Misaki's voice actress said the same in the official site's 2014 interview
    (`famille.novel.mfapps.jp/interview.html`, Wayback 2016-01-18): 「…他のキャラクターのお話を
    クリアしないと美咲さんのお話見られないみたいなんですっ」 ("it seems you can't see Misaki's
    story unless you clear the other characters' stories"). Saved in `research/wayback/`.
- **Photo window (MENU › Photo; The Technical Wizard's World `DBzQdN-tyn4` 3:57:10–3:59:07,
  English, 1080p):**
  - Title "Photo - Panel", with three tabs: **Photo Panel**, **Event**, **Epilogue**.
  - **Photo Panel** lists every login-panel picture won, one row per picture with her face icon.
    Tapping a row shows the picture at 3:4 between teal bars, then a caption fades in: "Aoi :
    Wake-Up Call / 2018/06/09-2018/06/23" and a blue **Close**.
  - Aoi's four: *Wake-Up Call* (2018/06/09–06/23), *In My Bed…* (2018/09/22–10/06), *I'm not
    crying…* (2018/11/21–12/05), *Changing Clothes* (2019/01/05–01/19). So each picture was the
    prize of one 15-day period, and the heroine changed between periods.
  - **Epilogue** lists "Epilogue - Part 1" and "Part 2", each opening to one row per heroine
    whose epilogue you've read (his: Misaki, Aoi). Aoi's Part 1 picture is her starry-night
    picture from the end of the route.
  - **Event** stayed empty for him.
  - `research/photo_pictures.py` cuts the five pictures out (upscaled 4×, 900×1200) into
    `img/photo/` and writes `server/data/photos.json`.
- **The login panel is a 4×4 grid (16 panels)**, not 9: 神崎LEN `kC7sdrz4JYw` 1:12 (Japanese)
  shows the picture behind 16 house-icon tiles with 「さやか：バカなんだから」 and 「パネル期間:
  2019/08/29-2019/09/12」 and a 「マイページへ」 button. The mock now uses 16.
- **The mock's Photo window** now has the three tabs. The current panel picture shows through
  the removed panels; finished pictures (handed out in order: Aoi's four) are listed and open
  in the viewer with their caption. Epilogue pictures appear once that epilogue has been read.
- **Japanese edition: the epilogue cost crystals.** When 神崎LEN finished Misaki's route
  (`tJTshlUQW5E` 5:20) a popup 「エピローグ出現!! これが本当のエンディング! 最高のエピローグを
  どうぞ!」 said 「恋結晶があと7個足りません…」 with 3 crystals owned, i.e. the epilogue cost
  **10 crystals**, and offered 15 crystals for ¥1,200. The "クリア!!" popup after it reads
  「おめでとう!!あなたのおかげで美咲さんもとっても幸せそうだよ。最後にエピローグもあるから
  しっかり見届けるんだよ!!」. Not seen in English footage: DevilGh0stz's final-chapter uploads
  end at "To Be Continued" and his epilogue uploads start inside the story. The mock leaves
  epilogues free.
- *(Corrected 2026-10-11: `fzu14-CAKcY` 1:36, Aki in glasses on a park bench, is an ordinary
  story part ending in "To Be Continued", not a touch scene.)*
- **Japanese story frames show the art cleanly** (portrait 720p). That may be a sharper source
  for story pictures than the English uploads.

**Storyboard sweep (2026-10-11).** Rather than downloading whole videos, `hunt/sb/` holds
YouTube's storyboards (one 160×90 thumbnail every ~10 s) for the 71 神崎LEN videos and ~190
other English/Japanese uploads (`hunt/len_playlist.txt`, `hunt/sweep_list.txt`).
- `hunt/sb_unpack.py` splits the `.mhtml` storyboards into timed thumbnails.
- `hunt/sb_flag.py` keeps the thumbnails without the dialogue box; `hunt/sb_rare.py` keeps only
  the screens that look unlike every other (the common home/popup/loading screens drop out).
  Contact sheets are in `hunt/sbflag/`.
- `hunt/grabframe.sh <id> <dir> <time>…` grabs full-resolution frames of the moments found.

**Result of the sweep:** all 71 神崎LEN videos and 184 other uploads (~47 h) were checked
through their storyboards (`sbflag/len*.jpg`, `sbflag/swrare_*.jpg`; the rest of the "rare"
screens are Android home screens, other games, webcams and story pictures). **None of them
opens the gacha screen or an Event Story.** The finds are in this section above (photo window,
16-panel grid, Japanese epilogue price, touch scenes).

**Second pass (2026-10-11 night): other kinds of sources**
- **Login-streak energy cap: found.** The full English tutorial is in PoISoN2462's "My Sweet
  Roomies Android time" (`EqwSQBUXVfE` 11:40–12:20, 1080p). Its "Login Bonus" page: *"Login
  everyday to increase your overall energy! Maximum Overall 8 Energy"*. The cap is **5 on the first
  day, 6 after 2 straight days, 7 after 5, 8 after 8**, and *"If you forget to login, your max
  energy will go down."* A Play-store review (soulextinguisher, 2016-07-12) adds "for some reason
  mine reset". The other pages: "How to Play", "What is Crystal?" (panel tickets, energy, *"Special
  Photos and Special Stories are also available!"*), "Dress Up with Gacha!" (*"Spin your Gacha and
  get a Dress Up Item! So many Dress Up Items!"*, the same six-outfit picture as the Japanese
  one), "And there's MORE…". **Mock:** `login_streak` column, cap = max(streak cap, course cap);
  a missed day restarts the streak.
- **English epilogues were paid:** PokeCommunity thread 398416, Squirrel McNutFace 2018-01-06:
  *"unless you pay to read the epilogue, you don't know how it's going to work out."* The
  Japanese price was 10 crystals (above). **Mock:** reading an epilogue the first time costs 10
  crystals instead of energy (402 with the shortfall otherwise).
- **Gacha price: 5 crystals per pull, 10 pulls for 50**, from the gacha screen of sister game
  恋Q部 (same developer Smart Novel Project, same engine, same 恋結晶; ハルク `EygEZekAZhU` 1:48–2:27):
  「恋結晶5個でガチャを引く」, 「10回連続で引く（恋結晶50個）」, a 「所持ガチャポイント」 counter, and a result
  「[服装]芹香の寝巻き をGET!!」 with 「ショップへ行く!」/「着せ替えする」. Famille's own screen is still
  unseen, but 5 matches the mock.
- **Event Stories (Japanese) — names and format from the archived @MF_Novel timeline**
  (Wayback 2014-08-23 and 2015-09-16, `research/wayback/tw_2014.html`, `tw_2015.html`):
  | Dates | Event | Notes |
  |---|---|---|
  | 2014-08-21 – ? | 海辺のシンデレラ (Seaside Cinderella) | slot-style event: spin to earn scenario pt and event pt for limited scenarios and rewards; a heroine wearing a **swimsuit** on My Page makes the slot better; swimsuits rate-up in the gacha; crystal sale |
  | (before 2014-08-12) | first time-limited gacha event | appget.com |
  | 2015-01-14 | もうひとつのプロローグ (Another Prologue) | slot event, how the six met / how Famille began |
  | 2015-01-28 | 彼女たちのバレンタイン (Their Valentine's) | |
  | 2015-02-13 | ファミーユ☆ポリス (Famille Police) | the heroines as police officers; 30 episodes, 3 epilogues |
  | 2015-02-27 – 03-08 | 株式会社ファミーユ (Famille Inc.) | everyone as office workers, new costumes |
  | 2015-03-16 | カノジョとケンカしちゃったら… (If You Fight With Her…) | 30 episodes; epilogue: Mitsuka's cute revenge |
  The banners are gone from Twitter's CDN; only a 150-px thumbnail of the last one survives
  (`wayback/tw_img/kenka_thumb.png`). No footage or text of any event story has been found,
  and none of this is known for the English edition.
- **Also checked, nothing new:** the APK (all UI is server-side HTML; only `se-system-gachaget.ogg`
  hints at the gacha), Nico Nico Douga (3 uploads, none relevant), the JP/EN store listings
  (Wayback; full Japanese crystal SKU list incl. 26 for ¥2,000 regular), applion reviews (20;
  one mentions buying events with a course), the official site's system image.

**Third pass (2026-10-12): the strategy wiki and the predecessor game**
- **famille.gamerch.com** (the game's Japanese 攻略 wiki, still online; text saved in
  `research/wayback/gamerch/all.txt`) has unlock tables for two heroines:
  - **Secret Stories (裏話) cost 5 crystals** each and open after a given chapter-part.
    Mitsuka: 1 「小さな思い」 after 4-6, 2 「浮かんだあの人」 after 10-5, 3 「初めての感情」 after 18-4,
    4 「痛む心」 after 25-2, 5 「残されたもの」 after 29-5. Sayaka: 1 「こいぬ」 after 3-3.
  - **Mid-route limited pictures cost 10 crystals** (Sayaka: 「謎の多い人 2を初めて読んだ際に限定イラスト
    出現【恋結晶10個】」), i.e. the 「次回限定イラスト発生!!」 popup.
  - Mitsuka had a 「前編エピローグ」 (first-half epilogue) after 32-5 for 10 crystals.
  - Mitsuka's 32 Japanese titles match the English ones one-for-one (3 「温かい食事」 = A Warm Meal,
    4 「マスコット」 = A Plush Toy), which gives the missing **Mitsuka Chapter 13: 「ふたりの水着ショー」**,
    added to `chapter_titles.json` as "The Swimsuit Show for Two" (my translation, marked with
    `title_source`).
  - **Mock:** the six Secret Stories are in `secret_stories.json` with translated titles
    (`"translated": true`, Japanese title kept). They have no text, so they play the placeholder.
    *(Superseded 2026-10-12: the English popup charges 5 crystals too; see "Sixth pass". The mock now charges 5.)*
- **The predecessor ただいまっ!うちカノジョ** (Mobile Factory, GREE from 2012-12-20, then Mobage, mixi,
  Ameba, iOS/Android `jp.mfac.tdim`; web versions ended 2017-12-15) is the same game without
  voices (Japanese Wikipedia). A 2013 iOS recording (inoue tamaki `mtlCHtI8VmM`) shows the same
  4×4 heart-tile panel and the same login page, which settles the missed-day rule:
  「ログインをサボると…チケット上限が5枚にダウン!」. Its web hosts (`gree.tada.mfac.jp`,
  `ameba.tada.mfac.jp`) were never archived beyond robots.txt.

**Fourth pass (2026-10-12): every unlock popup in 神崎LEN's 71 videos**
- `hunt/popup_batch.sh` downloads each 神崎LEN video (540p), runs `hunt/popups.py` (keeps every
  popup at 1 fps: orange title bar, or the green 「今はやめておこう」 button of an unlock popup, tagged
  `_S`), and deletes the video. `hunt/secret_sheets.py` makes sheets of the unlock popups (red
  「裏話出現!!/限定イラスト発生!!/エピローグ出現!!」 banner). 93 unlock popups in ~50 h of video.
- **Secret stories: 55 of 60 identified** with Japanese titles and unlock points — all 10 for Ren,
  Sayaka and Mitsuka, 10 for Aki (#4 「強く優しい決意」 only from a Memory list, L34 4:09), 9 for Aoi (#5 never shown), 6 for Misaki (7–10 not
  reached). Full list with timestamps: `research/ocr/hunt/pops/secrets_read.md`. Aki 1
  「素直になれない」 is the English "I Can't Be Docile", which ties the lists together.
- **Prices (Japan):** secret story 5 crystals, limited picture 10, epilogue 10.
- **Limited pictures (限定イラスト)** sit inside a story part. Aoi's four — モーニングコール, おれの
  ベッドで, 着替えの途中…!!, 泣いてないよ… — are exactly the four pictures recovered from the
  English Photo window ("Wake-Up Call", "In My Bed…", "Changing Clothes", "I'm not crying…"), where
  they were login-panel prizes instead. 15 limited pictures are named across the routes.
- **Mock:** `secret_stories.json` now has all 55 (built by `research/secret_list.py`); English
  titles other than Aki 1 are my translations (`"translated": true`, `title_jp` kept). A secret story
  opens after its exact part when known (`after_part`), else at the end of its chapter. Only Aki 1
  has text; the rest play the placeholder.
- **Aki's chapter numbering:** the Japanese edition has a chapter 36; English labels match the
  Japanese numbers on both sides of it (37 ハッピーデート! = "Happy Date!", 46 対面の時 = "The
  Meeting"), consistent with English having no 36.

- **No secret story was read on camera.** Grouping the title bars of all 4,601 saved popups
  (`hunt/pops/bar_groups.json`, sheets `bars_*.jpg`) found two Memory (思い出) visits: Sayaka's 裏話
  tab at L51 9:57 (he taps 裏話1 こいぬ but is 3 crystals short) and Aki's list at L34 4:01 (he
  replays main Chapter 25; the list shows Aki 4). So no secret-story text exists in this footage.

**Fifth pass (2026-10-12): route structure and the last missing chapters**
- **Two halves per route.** In every Japanese route a 「クリア!!」 popup with 「エピローグ出現!!」 appears
  mid-route ("まさかこれがエンディングなんて思ってるんじゃない？最後に取っておきのストーリーが残されて
  るわよ"), the progress meter restarts at 0%, and a second clear and epilogue come at the end. First
  halves end at: Ren 31, Aki 34, Mitsuka 32, Aoi in-game "34 Overcoming It All" (JP 33), Sayaka 32,
  Misaki 30. The recovered English text agrees: Ren's Epilogue 1 opens "It's been a few days since
  that fateful night", straight after Chapter 31's moonlit vows. **Mock:** Epilogue 1 unlocks after
  the first half, Epilogue 2 after the last chapter (`FIRST_HALF_LAST` in `app.py`); the main story
  never waits on them; the post-story popup offers an unlocked epilogue ("Epilogue N is here! The
  real ending… (10 crystals)"), and the Story button no longer auto-plays paid epilogues.
- **Missing chapters found:** Aoi 27 and Aki 36 (see section 3). Every main-story and epilogue part
  in the mock now has text.
- **Aoi's Secret Story 5** sits under Chapter 30 「揺れる想い」 (Aoi's Memory 裏話 list, L27); its title
  was never expanded on screen.
- **Limited pictures and the panel board (JP):** the 「イラスト - パネル」 screen lists several pictures
  at once with panels left ("アキ:ジロジロ見ないでよ… 残り:12", "蓮:雨に打たれて 残り:15"), so a limited
  picture not bought for 10 crystals went onto the login-panel board — which is how Aoi's four
  became the English Photo-panel prizes.
- **Login bonus (JP):** besides the daily panel, a counter of logins gives an item to the Present
  box: Silver Ticket ×1 or Panel Ticket DX ×1 / ×3 (「シルバーチケットをGETだよ！…次のログインボーナスまでは
  あと1回！」; other popups say あと2回 and あと4日, so the gap varies). **Mock:** every 2nd login day
  gives a Silver Ticket or a Panel Ticket DX in turn (a simplification of what was seen).
  Out of energy: 「恋結晶5個で今すぐチケット全回復!!」 (JP: 5 crystals refill all; the English shop
  sold 1 energy per crystal).
- **Famille's own gacha and dress-up screens found** (神崎LEN `2TnRB8p9sPc` = L59, 53:26–53:45):
  - 「ガチャ」: the banner 「恋結晶でガチャ 着せ替えアイテムGet!! 恋結晶5個」 (Misaki in a red coat, Aki,
    Aoi in a light dress, Mitsuka in a white yukata, Ren in black-and-white, Sayaka as a nurse),
    「所持ガチャポイント:0ポイント」 and, with 4 crystals, 「恋結晶が1個足りません…」 → **5 crystals a pull**.
  - 「着せ替えアイテム一覧」: tabs **表情 (expression) / 服装 (outfit) / 背景 (background)**, each a grid of
    ≥12 slots, locked ones shown as "?", the worn one ticked.
  - 「ショップ」: 恋結晶を購入する / アイテムに交換する / チケットを回復する / コースを変更する.
  - **Mock:** a separate Gacha window with that banner (`img/ui/gacha_banner.png`, upscaled 4×),
    gacha points and the 5-crystal pull; Dress Up has the three tabs with "?" slots (only outfits
    are modelled; the expression/background pools weren't recorded).
- **Checked, nothing new:** Wayback copies of the strategy wiki (2015–16), Yahoo!知恵袋 threads
  (one notes there was never a うちカノジョ wiki), archive.org (only a modded 1.4.6 APK), APKPure (1.4.4
  and 1.4.3 builds, same size), ハルク's channel (Part 10 is plain Mitsuka story).

- **Touch = chat bubbles, now for every heroine.** In the Japanese build, touching the heroine on the
  home screen shows a speech bubble over her (store page 「いつでもおしゃべり! 彼女にタッチしておしゃべりを
  楽しもう!」; English store page "Chat anytime! You can enjoy chatting with the girls with your
  fingertip!", with Aki saying "Hehe, thank you for caring me, but.. I'll be fine."). `hunt/bubbles.py`
  (`bubble_batch.sh`) found them in all 71 神崎LEN videos by looking for dark glyphs on a white box over
  the home screen (RapidOCR can't read this Japanese); in `uxrdNXkI-mU` 2:24–2:32 he taps every heroine
  in turn. **22 lines:** Ren 5, Aki 4, Mitsuka 5, Aoi 4, Sayaka 3, Misaki 1 — Japanese plus my English
  translation in `server/data/touch_lines.json`. **Mock:** a touch shows the next line in a bubble;
  for Aki, Sayaka and Aoi every third touch plays the recorded English touch scene instead.

**Sixth pass (2026-10-12 night): four lines of research in parallel.** Reports and evidence are in
`research/hunt2/<topic>/findings.md` (topics: english_footage, japanese_web, archives, editions_wording). The
brief they worked from is `research/hunt2/BRIEF.md`.
- **Two English channels that were never swept before** had what the earlier sweeps missed. Both buy secret
  stories on camera.
  - **SandDistress 83** (`UCFimGR1O92kM955qhxLmQ0w`): 197 videos from 2018, covering the Aoi, Sayaka, Ren and Aki
    routes.
  - **[Yu_Tsugami]** (`UCfwim5bqEcAg0iXTrt-wd9Q`): 86 parts covering Mitsuka, Aki and Ren.
- **Secret stories cost 5 crystals in English.**
  - The popup: "Get the Secret Story!!" / "Get the Secret Story behind the scene!" / the title / then either
    "You need N crystal(s) more!" or "The total amount of Crystal : 5 → 0" with "Get it NOW!".
  - **Mock:** `SECRET_COST = 5` in `app.py`; too few crystals gives a 402 with "You need N crystal(s) more!".
- **Official English secret-story titles: 49 of 56**, read from those popups.
  - The list is in `english_footage/titles_found.md`. The strings are exact, including the game's quirks, such as
    "Secret Story - Cowardly Love" with no number.
  - The other 7 (Ren 9, Aoi 5, Misaki 1–6) come from a **Wattpad rewrite of all six routes** (Someonewhosnotfamous,
    2016–18). Its titles match the official ones wherever both exist, e.g. Aki 4 "Strong But Ephemeral Decision".
    The only difference is Ren 2, which is officially "The Window".
  - Aoi 5 = "Aoi's Wish".
  - Misaki 7–9 = "Invitation", "The Dare", "The Last Night". They aren't in the mock because their unlock points
    are unknown.
  - Every entry in `secret_stories.json` has a `title_source`.
- **Secret-story text: 2 of 56.**
  - **Aki 1 was wrong until now.**
    - The mock played HDBarbecho 8:40–12:16, which is *main* Chapter 4 part 4.
    - The real secret story is 5:05–8:40: Aki's room after the shopping street, where Mitsuka talks her into
      thanking William.
    - I read its 49 lines by eye, because the OCR skips the yellow thought boxes.
    - The old version is kept as `ocr/out_v1/Aki_S01_wrongscene.json`.
  - **Aoi 1 "Day Before Plan":** 46 lines from SandDistress `swmPbeU6CPQ` 5:50–7:42, in `ocr/secret/Aoi_S01.json`.
  - The Wattpad rewrite has fan *paraphrases* of 59 secret stories, with the characters renamed
    (`editions_wording/wattpad/secret/`). They aren't used in the mock.
- **Chapter titles (official, from "next story" popups):**

  | Chapter | Official title | Source | Mock had |
  |---|---|---|---|
  | Mitsuka 13 | "The Bikini Show" | `IWbg66S7cYk` 1:34 | my "The Swimsuit Show for Two" |
  | Aoi 54 | "Our Happiness" | `RLzuZjEvJQA` 13:56 | the same (my translation matched) |
  | Sayaka 48 | "Brief Respite" | `XIHGZTQW4Os` 4:55 | "A Fleeting Calm" |

- **Chat bubbles: all 82 voiced lines.**
  - The **Japanese 1.4.6 APK** ships `Application/sound/voice/mypage/*.ogg`: Ren 13, Aki 14, Mitsuka 19, Aoi 15,
    Sayaka 14, Misaki 7. The English APK has none.
  - The client's `/native/speak?message=<text>&voice=<id>` shows the line and plays that clip.
  - All 22 lines read earlier match a clip.
  - **25 official English lines** were read from English uploads (Aki 9, Aoi 6, Sayaka 7, Ren 1, Mitsuka 2). The
    other 57 are my translations of the Japanese, which is a whisper transcription (`jp_asr: true`).
  - **Mock:** `touch_lines.json` has all 82, each with `voice` and `en_official`; it's built by
    `research/hunt2/merge/build_touch_lines.py`. The clips are in `data/static/sound/voice/mypage/`, and the page
    plays them with HTML5 audio.
- **The English gacha is called "Machine"** (Chicoine `Dgd2gg2wIa0` 12:06–13:36).
  - Banner: "Spin with Crystal! Get Her Premium Outfits!! 5 Crystals". Counter: "Machine point(s)".
  - Help page: duplicates give Machine Points, "You can spin the machine once with 10 Machine Points", and "there
    will be additional Machine for a special event". A spin can also give "items like a energy ticket".
  - Dress Up tabs: **Face / Clothes / Background**.
  - **Mock:** the wording is applied.
- **Other English text:**
  - Epilogue popup: "Get the Epilogue!! Finally!! Enjoy the last special epilogue!!" (10 crystals).
  - Route clear: "Cleared! Thanks to you, Sayaka is happy! But... Don't think you're finished yet. There's still
    one last, special story waiting for you."
  - Special-photo titles for Aki, Ren, Mitsuka, Sayaka and Aoi are in `english_footage/findings.md`.
- **An earlier English edition: "Honeys, I'm home!"** (Mobile Factory, iOS, 2014-03-31, id797418099), the unvoiced
  English うちカノジョ.
  - Chips unlocked Secret Stories and the final episodes.
  - In-app purchases: Secret Story Ticket $2.99, Epilogue Ticket $4.99.
  - Its store text: "After certain events, see just what she was thinking with Secret Stories!"
- **Event stories: still no text anywhere.**
  - New names from the predecessor, via the Mobage fan circle `sp.mbga.jp/_grp_view?g=32825817`:
    二度目のバレンタイン, White Day 2013, クリスマス狂詩曲〜ラプソディー〜, バレンタイン第一弾, and the last event
    『Ma chère famille』. Three banner thumbnails are saved.
  - The developer's admin paths include `/story/scratch_event_story/import_data`, so events were probably
    scratch-card events.
- **Server archives: none.** Wayback, archive.today and Common Crawl have no capture of the game or asset hosts
  beyond the known 401, and the hosts have no DNS today.

- **Mobage circle, logged in (2026-10-12)** (`research/hunt2/yakumo/findings.md`): joined with my own account. There's no story text anywhere: 八雲's album is empty, his game diary was deleted, and the 7 images
  exist only at 320 px. A message asking him for his screenshots is drafted in `yakumo/msg.txt`. A friend request to him is pending.
- **Mock: the Event Story tab now lists the 11 known events** (`data/event_stories.json`: 6 『ファミーユ』 events from
  the archived tweets, 5 うちカノ events from the circle). Each row shows its game and date, and a banner where one
  survives (3 うちカノ banners from the circle, the 150-px 「カノジョとケンカ」 tweet thumbnail). None is playable,
  because no event text exists. The English titles are my translations. API: `/api/events`.

**"Mega Assets" pack (2026-10-12, a fan-made pack; copied to `research/mega_assets/`, 52 MB, 611 files).**
A fan collection from a Google Drive folder "My Sweet Roomies! Mega Assets".
- **`Outfits/`:** official Japanese event and gacha cards (~564×745), with a README by the collector. It says the
  game drew each heroine with two head angles (front and 3/4), expressions as transparent overlays, and one body pose
  per outfit. Sets:
  - Idol (ファミーユ☆スクールフェス)
  - Kimono (カノジョとふたりですごす夏)
  - Maid (ようこそファミーユカフェへ)
  - Officer (ファミーユ☆ポリス)
  - Swimsuit (海辺のシンデレラ)
  - Wedding Day (彼女がドレスに着替えたら)
  - Unsorted: about 60 more cards and home screenshots. They name further events (カノジョとケンカしちゃったら,
    もうひとつのプロローグ, 株式会社ファミーユ, White Day, プロポーズをもう一度, 山なでしこ…) and show Aki in many outfits.
- **`CGS/`:** story and limited pictures; seven at 1080×1920.
- **`Random/`:** 1440×2560 Vietnamese store screenshots (Aoi's default art drawn large) and a **1080×1920 home
  screen from a Traditional Chinese edition** (「戀結晶」, 「亞紀」; a Facebook button). That edition wasn't known
  before.
- **Other folders:**
  - `voice/`: the same 82+6 clips as the Japanese APK.
  - `sound effects/`: 32 `se-*.ogg`.
  - `Game Assets/`: the game's own font `07YasashisaGothic.ttf`, particle plists (sakura, snow, rain, kaede,
    clover), shaders, the title screen `top_bg.png` / `top_char.png` / `logo.png`, Android UI drawables.
  - `Alt Name-title screens/`, `Game app Icons/`.
- **Why the outfits are cut out:** the home screen draws the heroine as a transparent image over the house
  background, with the buttons on top, so an outfit has to be a see-through picture of just her. The cards are
  finished promo art: event background, a close-up of another heroine, titles, logo, decorations. The game's real
  outfit layers (body per outfit, two head angles, expression overlays, per the pack's README) were served from
  Mobile Factory's servers and didn't survive, so a cut-out is the closest stand-in. Cutting at the waist matches the
  home-screen framing; the rest is under the buttons.
- **Limited pictures in the Photo window (2026-10-13, `research/cg_photos.py`):** 10 of the `CGS/` pictures are
  now login-panel pictures after Aoi's four (900×1200, like hers). The story-viewer frames have their teal bars
  cut off; the cards are upscaled 2×.
  - **Titled, matched to 神崎LEN's popup titles** (`ocr/hunt/pops/secrets_read.md`):
    - Ren: イタズラな風… ("A Mischievous Breeze…", wind and petals) and 雨に打たれて ("Caught in the Rain").
    - Sayaka: 優しいさやかさん ("Kind Sayaka", holding a puppy). Her first secret story is 「こいぬ」, so this is
      probable, not certain.
    - The English titles are my translations.
  - **Untitled:** Aki ×3, Mitsuka ×2, Sayaka ×1, Aoi ×1, captioned "Title not recorded".
    - Unconfirmed guesses: Aki's surprised ponytail picture may be このっ、変態いぃー!!, and her shy one on the
      sofa ジロジロ見ないでよ.
    - The Mitsuka picture at night (`928146c4`) has grey-navy hair. That's her, not Aoi.
  - **Not used:**
    - two Japanese copies of Aoi pictures already recovered in English (`42afabc3` In My Bed, `ac5a1c1f`
      Wake-Up Call);
    - a duplicate card (`1937f968` = `5181042f`);
    - the six-heroine group picture and Misaki/Sayaka swimsuit picture (event art);
    - two うちカノ banners.
- **Event Story banners and nine more events (2026-10-13, `research/event_banners.py`):** each 『ファミーユ』 event
  had promo cards with its logo, one per heroine. One card per event is now its banner (`img/event/<id>.jpg`).
  - **Events named only on the cards** are added to `data/event_stories.json`, 20 events in all. Their dates are
    unknown and the English titles are mine:
    - ファミーユ☆スクールフェス, カノジョとふたりですごす夏, ようこそファミーユカフェへ, 彼女がドレスに着替えたら
      (the outfit sets);
    - 癒やし癒され温泉紀行, プロポーズをもう一度, a 『ファミーユ』 White Day, 山となでしこ, 新婚レッスン♪
      (`Unsorted/`).
  - **Banners:** every 『ファミーユ』 event has one except 彼女たちのバレンタイン. Kenka's card replaces its 150-px
    tweet thumbnail.
  - **`Unsorted/` cards not used:**
    - cards with no event logo, on a "family / our child" theme (maybe the newlywed event);
    - ~15 Japanese home screenshots (~490×880) of Aki in event outfits: Santa, witch, gothic dress, police, robe,
      parka and others. **These are a lead for more of Aki's outfits.**
- **Title screen, font and sounds (2026-10-13, `templates/index.html`):**
  - **Title screen (rebuilt from the APK's own layout the same day):** the APK has the title scene's
    CocosBuilder file `assets/Application/StartGameSceneLayer.ccbi` and its images in
    `ccbResources/start_game_scene_layer/`. The new `research/ccbi_dump.py` reads `.ccbi` files (a port of
    cocos2d-x's CCBReader, format 5). It shows:
    - a white layer;
    - `top_bg.png` fades in at 1.5–3.5 s;
    - `logo.png`, anchored bottom-right at 98% / 80% of the 320×568 screen, rises from 75% while fading in at
      3.7–4.87 s;
    - `top_char.png` (the six) fades in at 4.5–5.43 s;
    - a pink **"Start the Game! My Sweet Roomies!"** button (`btn_start.png`, scale 0.45, centred at 50% / 23%)
      wiggles at 5.07 s;
    - round **Contact** and **Option** buttons sit at the top left (7.5% / 95%, 20% / 95%);
    - a hidden debug button;
    - the timeline's `onTitleShowCallback` at 4.03 s;
    - feather and sparkle particles (`feather.plist`, `kirakira.plist`), which the mock leaves out.

    `NoticeSceneLayer.ccbi` is `top_bg` with the notice fading in at 0.17–1.03 s.

    **Title music:** the pack's "Opening Track.ogg" is byte-identical to the APK's `sound/bgm/splash/13_01.ogg`,
    the only BGM the *original* APK ships. (The offline build also carries the official soundtrack — see
    **Soundtrack** below — but the title screen still plays this track, as the game did.)

    **The mock** (`templates/index.html`) follows this exactly.
    - The game starts on the START button; the notice follows on first use.
    - The music starts at the 4.03 s callback, or on the first touch if the browser blocks autoplay. It loops and
      fades out on entering the home screen.
    - Option toggles the music (remembered per browser); Contact says it isn't available.
    - The title shows once per browser session and never on a return from a story.
  - **Opening movie and other editions' title screens (2026-10-13):** on a fresh start (a new browser session,
    i.e. relaunching the app), the mock chooses at random.
    - **Opening movie, half the time:**
      - English: the APK's `video/opening.mp4`, also the pack's `opening.mp4`;
      - or Japanese: the pack's `opening ver2.mp4`, the original with kanji names and voice-actor credits.
      - Tap skips the movie. If the browser started it muted, the first tap turns on the sound.
    - **Then one of five title screens:**
      - the English one above;
      - or a screenshot of another edition's title (`Alt Name-title screens/`): Love Rock × うちカノ, *Honey I'm
        Home!* (the predecessor's English edition; iPod status bar cropped), Full House (Vietnamese; black bars
        trimmed), 女子宿舍禁愛令 (Traditional Chinese).
      - On a screenshot title, a touch anywhere starts the game.
    - **Files:** `static/video/opening_{en,ja}.mp4`, `static/img/title/alt_*.jpg`.
    - **For testing:** `?intro=en|ja|title|alt1..alt4` forces one choice.
  - **Font:** the game's font `07YasashisaGothic.ttf` is now the page's default text font
    (`static/fonts/yasashisa.ttf`). The rounded and handwritten fonts rebuilt from the recordings stay on the
    home-screen labels.
  - **Sounds:** `se-system-button` when a window opens or on the title tap, `se-system-close` when a window
    closes, `se-system-bonus` on a login bonus (`static/sound/se/`). `se-system-gachaget` was already in use.
    The story sound effects (door, footsteps…) aren't wired up, because the transcripts carry no sound cues.
  - **Soundtrack (2026-10-16):** the official **My Sweet Roomies! Original Soundtrack** (21 tracks: the main
    theme, each heroine's one-to-three character themes, and seven mood pieces — Common, Date, Recollection,
    Noisy, Anger, Painful, Sadness) is now in the game. `offline/convert_ost.py` transcodes the download's MP3s
    to the engine's BGM format (Vorbis `.ogg`, like `sound/bgm/13_01.ogg`) under
    `static/sound/bgm/ost/<slug>.ogg` and writes the catalog `data/soundtrack.json`.
    - **Story music.** The original per-scene tracks were downloads from the live server and are lost, so
      `engine_format` (app.py) now opens each story on **its heroine's first character theme** (`HEROINE_BGM`);
      the shared prologue and any heroine-less story use the main theme (`MAIN_BGM`). This is a best-effort match,
      not the exact in-game cue. As before, the native engine plays `Application/cache/` + `audio.bgm`, so the
      offline APK carries every track there (`offline/build_apk.py`, stored uncompressed); the PC server stays
      silent for stories (no native engine).
    - **Music room.** A new **Soundtrack** item on the home MENU opens a list of all 21 tracks grouped main /
      character / mood; tapping one loops it on an HTML5 `<audio>` (the title BGM is paused so they don't overlap,
      and it stops when the window closes). Served from `/api/soundtrack`; in the offline APK the Java server
      answers the same route from `static.json` (`export_data.py`, `Game.java`).
- **Used so far:** Ren's six outfits (`research/ren_outfits.py`, via the new `research/cut_anime.py`, which runs
  background removal at the CPU cap, with options for crop, upscale, colour drop, erosion and top-bar
  repair), and on 2026-10-13 the other heroines' 19 (`research/mega_outfits.py`; section 3
  "Dress Up").
- **Not used yet:**
  - Aoi's portrait from the Vietnamese screenshot

**Seventh pass: the Traditional Chinese edition (2026-10-13, `research/hunt3/`)**
The game also ran in Taiwan as **《女子宿舍禁愛令》** (LEGAME / 玩拾科技, package `legame.mfapps.novel.famille`,
launched 2016-06, closed **2017-12-16 18:05**; servers `android.famille.mf.perfecten.net`,
`cdn.famille.mf.perfecten.net`). Its fan community left far more behind than the English one.
- **Bahamut board** 女子宿舍禁愛令 (`forum.gamer.com.tw` bsn=30457): all 180 threads saved
  (`hunt3/baha/threads/`, parsed into `all_threads.txt`; `grab_all.py` backs off on HTTP 429). 106 of the
  thread images are deleted from Bahamut's storage; a full Wayback check (after its 2026-10-13 outage) recovered none.
- **A full wardrobe dump.** On the last day a player (魯魯藍黑斬, snA=182 #5) screenshotted their whole wardrobe
  and shared it as Google Drive folders that are **still live**. Downloaded to `hunt3/drive/<id>_<name>/` (gdown):
  per heroine the Character card, the Dress Up grids, **~24 home-screen shots in every owned outfit (144 in all)**
  and the limited CGs (26).
  - `research/tw_wardrobe.py` sorts them (`classify` → `hunt3/drive/index.json`) and cuts every outfit
    (`cut`). The translucent top bar is undone with a fit over all 144 shots
    (`fitbar`: ~65% opaque, red tint varying left to right). **125 outfits are in the mock** (section 3 "Dress Up",
    "Taiwan-edition wardrobe").
  - **Misaki's Character card** was on it: 166cm/47kg/B86/W59/H87, quote and profile (now in `heroines.json`,
    my translation). It was the last missing card.
  - **Dress Up facts** (snA=182 #1): **9 expressions** (初始 default, 笑顏 smile, 生氣 angry, 哭泣 crying,
    害羞 shy, 驚訝 surprised, 困惑 confused, 瞇眼 squint, 超害羞 very shy); **13 backgrounds** (outside the house
    morning/evening, each of the six heroines' rooms, living room, balcony morning/evening/night, school
    cafeteria); outfits **Ren 30, Aki 29, Mitsuka 30, Aoi 29, Sayaka 31, Misaki 30**. Misaki's can't be drawn from
    the gacha until her route is unlocked. The expression items themselves exist only as small grid icons.
  - **CGs:** 13 not seen before are added to the Photo window (`cg_photos.py`, `TW_PICS`), 3 titled by matching
    神崎LEN's popup titles (Ren 密着ウォータースライダー, Sayaka 真っ赤なドレス, Misaki ズッコケ美咲さん) and 3 event
    pictures (shown in the Photo window's Event tab). The CG filed as Sayaka `05_limited_a` is Aki's (now
    `02_limited_d`).
- **Secret stories: 15 new texts.** YouTube recordings linked from the board (Bahamut user 彤's channel qqabc2006
  and DanteChow): **Aki 1–3, Sayaka 1–10, Misaki 1–3**. Read from 1.5-fps dialogue tiles by eye
  (`hunt3/tw_secret/tiles.py`; `zh_ocr.py` OCR as a cross-check) and translated into English by me in the
  style of the official Aki 1, which the Chinese Aki 1 matches line for line. Players' names normalised to
  William. `research/tw_secret_stories.py` writes the transcripts. Gaps: Misaki 2's recording stops at
  "William!!"; Misaki 3's starts after its opening lines; Sayaka 4 may lack its last line. **Secret stories
  with text: 17 of 56** (was 2).
- **Event: 大江戸純愛物語 (Edo Pure Love Story)**, a gacha event (TW run from 2017-01-19): its **preview story**
  (V9e3_sRj-No) is recovered and **playable from the Event Story tab** (scenario `530008010101`, banner from a
  board image). Event mechanics from the board: normal stories earn event gacha tickets that unlock the limited
  stories, 20 parts per heroine. Other TW-era events named on the board: 和她一起共度的夏日祭, 海邊的仙杜瑞拉,
  溫泉戀旅甜蜜行, 女僕咖啡廳, 兩人的危險聖誕節 (a Christmas event not known before).
- **Edo event gacha and Sayaka's episode 1 (2026-10-14).** 彤's "大江戶純愛物語轉蛋" (`HFtNCIJ2i1Q`, 99 s,
  `hunt3/tw_long/`) shows the event gacha, then Memory > Event and **Sayaka's Edo episode 1 read in full** (16 lines,
  "To Be Continued"). `research/tw_edo_gacha.py` holds the transcript, my translation and the gacha facts; the episode
  is **playable** as the Edo event's second part (scenario `530008050101`; the Event Story tab lists the event's
  parts under it). What it shows:
  - Event gacha: banner, ticket count 「擁有轉蛋票券: 26張」, **5 tickets per spin** or 10 spins for 50; confirm
    「轉蛋票券5張 確定使用進行轉蛋嗎?」 [轉蛋!] [不行，還太早呢。]; the capsule machine with Skip; result 「朝彌香 1話
    獲得!!」 with "read this story" and "read stories to earn gacha tickets". **A spin gives the next episode of one
    heroine's event story** (this video: Sayaka 1, Aoi 1, Aoi 2, Ren 1, Ren 2), not an outfit.
  - Memory (回憶) tabs: 本篇 / 秘聞 / 限時劇本 / 活動 (main / secret / limited-time / event).
  - Finishing an event episode shows 「朝彌香與我的回憶」 with her line 「那個時候真的很快樂呢～！以後也一起創造很多快樂的回憶吧♪」
    and 「回到一覽」.
  - Episode 1 itself: on a day off Sayaka invites William for a drink "somewhere a little different", revealed tonight.
- **Long TW playthroughs checked (2026-10-14):** the Aoi, Mitsuka and Sayaka playlists in `hunt3/tw_videos.tsv`
  (59 videos, 11 h) show no secret stories, only main story (section 5 step 7.3).
- **Other 彤 uploads checked (2026-10-14):** the channel's other Famille videos are Sayaka's main route and her secret
  stories (already done). `L5KJgViOgsw` "重溫劇情" from the board's list is MapleStory footage.
- **Recordings still to mine:** 彤's channel has Sayaka's whole route (319 parts) and Aki's (97) in Chinese, plus
  an Edo gacha video; the Mitsuka and Aoi playlists linked in snA=196. The English text of those routes is
  already complete, so only event and secret material matters.
- **TW APK 1.0.22** found on itmop.com (`hunt3/apk_tw/`). Compared with English 1.4.6 it only adds the 82
  mypage voice clips (already in the mock) and the font; story art and dress-up layers were server-side here too.
  Its servers aren't in Wayback (only 7 voice samples on the CDN) or Common Crawl.
- **Older English builds:** none before 1.4.3 can be found. APKCombo and HappyMod no longer exist (their domains
  are gone), APKPure blocks scripts, Aptoide has none, and Wayback/AppBrain list only 1.4.3–1.4.6.

**Eighth pass (2026-10-14): more Taiwan-edition recordings (`research/hunt4/`)**
A new YouTube sweep (`hunt4/yt_search.py`, about 60 queries in Chinese, Japanese and English; `yt_all.tsv`) and the
uploads of each channel it hit (`hunt4/chan.py` -> `chan_lists.tsv`; descriptions in `desc.json`) found channels the
earlier passes missed:
- **Lu Dae** (188 videos, ~17 h): whole Ren, Aki, Mitsuka, Aoi and Sayaka routes, labelled by voice actress
  (大西沙織 Ren, 安野希世乃 Aki, 木戸衣吹 Mitsuka, 田所あずさ Aoi, 石上静香 Sayaka), plus **Mitsuka's secret stories
  1–6 (`QQdftsqS9Yk`) and 7–10 (`b3JRIrhpbpI`)** played from the Memory list.
- **單機女王CK Game Channel** (49): Ren and Mitsuka routes; Ren's parts 4, 5, 7, 10 and 14 include **Ren secret
  stories 1–5** (titles in the video names).
- **大熊猫中国** (101): includes **the Christmas event 「兩人的危險聖誕節」 in 21 parts** (~45 min).
- **breeze llc**: two parts of a **hot-spring special event** (「特別活動：與朝彌香的溫泉之旅」, 「溫泉之旅葵篇」).
- 夏索 (77 short Mitsuka clips, one with secret story 1), Kwun Tang Kwai, King (Misaki route, 42), 開膛手傑克 (Aki),
  A機 (34 Cantonese VTuber streams, #7 "活動"), and a few single uploads.
Downloaded to `hunt4/vid/` (≤720p): both Mitsuka secret videos, the 5 Ren parts, both hot-spring parts and 20 of the
21 Christmas parts (`jobs.tsv`, `get.sh`; `xmas_02` and `xmas_21` not fetched yet).

**Mitsuka's and Ren's secret stories (done 2026-10-14, in the mock).** Read by eye from dialogue-box tiles
(`hunt4/tiles2.py <video> x0,y0,x1,y1 <out> <fps>` -> `hunt4/vid/tiles/`; Lu Dae's 360 px video uses box
`0,404,360,536` at 2.5 fps, CK's 406×720 video `0,420,406,590` at 2 fps; the RapidOCR pass was too rough and was
dropped). Transcripts with my English translation: **`research/hunt4/mitsuka_secret.txt`** (stories 1–10, ~360
lines) and **`research/hunt4/ren_secret.txt`** (stories 1–5, ~160 lines). `research/tw_secret_stories2.py` writes
them to `server/data/scenarios/<id>/transcript.json` and sets `text_source` in `secret_stories.json`. All 15 TW titles
match the official English titles. **Secret stories with text: 32 of 56.** Test: `test_eighth_pass_secret_stories`.
Notes:
- The recordings replay a few lines (dropped). Story 4's title card wasn't captured, so 「秘聞4：心痛」 comes from the list.
- Mitsuka's stories are told from her side; Ren's from Ren's. Each of Ren's sits at the end of a CK route part,
  after the purchase popup. Ren 5 is mid-video (part 14, 5:09–7:03).
- Aoi calls Mitsuka 小光 (from her TW name 光夏); the mock gives it as "Mitsuka".
- Mitsuka 10 ends on a cliffhanger ("Wait for me... William!!"), as the recording does. Ren 2 may be missing one
  Misaki line at 13:40 (merged away by the tiler).
- **The TW purchase popup** (CK part 14, 5:03) reads 「秘話出現!!」 "想知道隱藏在目前故事的秘聞嗎？「秘聞5：顫抖的心」",
  with the price shown as 戀結晶 (Love Crystals) **15 → 10**, a 「首儲☆50%折扣!」 banner, and the buttons
  「立刻獲得!」 / 「現在先不要」. The English edition's price is 5 crystals, which the mock keeps.

**Hot-spring event: Sayaka's and Aoi's routes (done 2026-10-14, in the mock).** breeze llc's two "特別活動" videos
are one event, 「去溫泉戀愛旅行吧」 ("Let's Go on a Hot-Spring Love Trip"), 20 episodes per heroine (title cards
「去溫泉戀愛旅行吧 n/20」). Both routes are read in full: **`research/hunt4/hotspring_sayaka.txt`** (~430 lines; 1280×720
video, box `462,420,818,586`) and **`research/hunt4/hotspring_aoi.txt`** (~455 lines; 240×480 video, box `0,280,240,392`).
- Sayaka: a day-trip date turns into a night at a dog-friendly resort inn. There's a borrowed puppy, a dog run, an
  open-air bath in swimsuits, and her dream of "a kind husband, cute children and a lively little dog". It ends on a kiss.
- Aoi: her live-in job at a hot-spring inn runs over. Sayaka teases William that she has fallen for someone else, so he
  goes there and ends up working beside her. The truth: she was earning money to treat everyone from Famille to the
  inn as a surprise. The landlady sends them to the "Matchmaking Hot Spring", which turns out to be a foot bath. It
  ends with talk of a honeymoon, and Famille arriving.
- A few title cards weren't captured: Sayaka 3, and Aoi 1, 12, 15 and 17. The parts still run on directly, so their
  boundaries come from the recording's breaks.
- Which Japanese event this is isn't known. The TW board lists 溫泉戀旅甜蜜行; the JP cards show 癒やし癒され温泉紀行
  (a yukata set, while this event has swimsuits). It's a separate entry, `famille_hotspring_trip`, with no banner.
- `research/tw_hotspring.py` writes 40 scenarios, `530009<heroine><part>01`, and the event's `parts`. Each episode ends
  "To Be Continued" except part 20, which is marked `"final"` and ends on Fin (`EVENT_EPISODES` in app.py skips
  final parts). Test: `test_hotspring_event_routes`. Event Story now lists 22 events.

**Problem found and fixed: the tiler dropped lines.** The first `tiles2.py` compared 96×32 thumbnails, so short
boxes that look alike when shrunk ("......" then "...!!", or the same speaker's next thought) merged, and their lines
were lost. **Fix:** a new box now starts when more than 1% of the crop's pixels change by more than 60 grey levels.
`hunt4/missed.py` lists only the boxes the old detector missed, each with the box before it. Using it:
- Ren 2, 4 and 5 were re-read in full.
- Mitsuka 7 and 9, Ren 3, and both hot-spring routes were patched (about 30 lines each for the hot-spring routes).
- Mitsuka 1–6 and Ren 1 had nothing missing.

**Christmas event: Ren's route (done 2026-10-14, in the mock).** 大熊猫中国's 21 videos are one episode each of
「兩個人的危險聖誕節」 ("A Dangerous Christmas for Two"), Ren's route, 20 episodes. **`research/hunt4/xmas_ren.txt`**
holds ~550 lines; the 406×720 videos use box `0,420,406,590` and the two 360×640 ones `0,378,360,512`. `xmas_02` and
`xmas_21` were fetched anonymously on the second try; `xmas_05`, `15`, `20` and `02` are video-only files, which is
all the tiles need.
- **Story:** after a film about a hotel maid, Ren secretly takes a job at the maid café 'White Cat' (from the main
  route's maid episode) to pay for a surprise Christmas Eve at a luxury hotel. Hiding it ("buying a maid manga",
  "studying at the library") hurts William, and Sayaka's teasing makes it worse. She turns down his invitation to
  see the illuminations, and things crack. On the night itself she is kept late at the café, her phone is dead, and
  she stops to help a man with anaemia on the train. That makes her miss the 10 pm close of the café where they were
  to meet.
- **Gaps:** episode 2's opening lines, and everything after 2:00 of the final episode (the recording stops), are lost.
- **Mechanics** (xmas_21 help screens, `xmas_ren.txt` notes): this is a scratch-card (刮刮樂) event.
  - Reading stories earns scratch-card points, and sometimes a "FEVER".
  - Scratching gives event points to the heroine shown on the home screen.
  - Event points unlock her episodes, from 110 pt for episode 4 to 1000 pt for episode 20, and a limited picture
    (蓮の限定イラスト) at 1200 pt. They also rank the player.
  - The home screen adds 「特殊衣裝確認」 (check the special outfit), and the completion card shows Ren in a Santa outfit.
- **Bonus:** seven of Ren's TW home-screen chat lines are noted in `xmas_ren.txt`; they are not yet in the chat list.
- `research/tw_xmas.py` writes 20 scenarios (`53001001<ep>01`) and the event `famille_dangerous_christmas` (no banner,
  JP title unknown). Test: `test_christmas_event_ren_route`. Event Story lists 23 events; tests: 31 pass.

**Eighth pass: summary.** It added 15 secret stories (Mitsuka 1–10, Ren 1–5; **32 of 56 now have text**) and two
events (hot-spring: Sayaka and Aoi, 40 episodes; Christmas: Ren, 20 episodes), about 2,300 lines in all.
**Next leads:** the other heroines' routes of both events (the Christmas list shows 「光夏:浴室事件!?」 for Mitsuka),
breeze llc's other uploads, and secret stories inside the Lu Dae / CK route videos for Aki, Aoi and Misaki.

**Eighth pass, part 2 (2026-10-14 night): finding secret stories and events in long recordings.**
- **Two scanners** (`research/hunt4/`):
  - `titles.py [--crop x,y,w,h] video...` OCRs the first narration box after every black loading screen (RapidOCR,
    capped to 8 cores). That box holds the part title: 「第n章…」, 「秘聞n：…」 or 「<event> n話」.
  - `popups.py [--crop …]` finds the secret-story purchase popup (red 「立刻獲得!」 above green 「現在先不要」) at 1 fps.
  - Use both: the popup shows even when the player declines, and titles catch purchases the popup check misses.
    Letterboxed videos need `--crop` set to the game screen.
- **Results:**
  - CK's Ren second half (10 parts): no secret stories 6–10; CK stopped buying after story 5.
  - breeze's Aoi route (20 parts + bonus): **Aoi secret story 2** 「秘聞2：再來的話」 (= "One More Time"; part 5,
    17:45) was bought and is now read (`aoi_secret.txt`, in the mock via `tw_secret_stories2.py`). The popups for
    Aoi 6 and 9 (parts 12 and 15) were declined. **Secret stories with text: 33 of 56** (23 after the Mitsuka removal, 2026-10-15).
- **A機 (Hong Kong VTuber), 34 streams of the TW edition, July–August 2016** (`vid/ama/`, 480p, the best available;
  game screen at x 292–562):
  - **Seaside Cinderella** (海邊的仙杜瑞拉, JP 海辺のシンデレラ, a slot event; a Famille trip to Okinawa). The episodes are
    spread over streams #7–#19 and #29–#32, replayed from Memory > Event. Found with `titles.py` (title cards) and
    read from clips (`hunt4/cliptiles.sh <stream> <start> <dur> <prefix>`). **71 episodes** are in `seaside.txt`:

    | Heroine | Episodes read | Missing | Story |
    |---|---|---|---|
    | Aki | 1, 4–8 and 10–20 | 2, 3, 9 | She shuns the sea because her first idol shoot, live in Okinawa, ended with her falling in. William shows her the glowing reef at night and the sunrise. |
    | Mitsuka | — | — | Episodes 1 and 4–20 were in the streams; not translated (see below). |
    | Sayaka | 1–3, 5–20 + her event secret story | 4 | Swimsuit shopping, William's cut foot, her heatstroke, the lap pillow, the swim-ring day. |
    | Ren | 1–3, 5–16 + her event secret story | 4, 17–20 | The sandcastle "fortress", the flirts, the trampled castle, the names in a heart. |
    | Aoi | 1, 3, 4, 7–12 | 2, 5, 6, 13–20 | Her lost best friend from Okinawa, the coral, the sparklers promise. |
    | Misaki | 1–4, 7, 9, 10 + her event secret story | 5, 6, 8, 11–20 | Waking in William's suite, the neighbouring single rooms, the horror film, her jealousy on the beach. |

    Also read: the **early-clear bonus side story** 「早期攻略・番外篇」, in which Aki and Aoi collect star sand on a
    private beach.
    - **Event secret stories:** 「…的秘聞」, unlocked with 祕聞點數 (secret points) from the slot. Keys `x99`; the
      side story is `298`. In the mock they are titled "<Heroine> Secret Story" and "Early-Clear Bonus Story".
    - **Common epilogue:** Memory lists 「共通尾聲」 after episode 20. It isn't in the streams.
  - **Stream #26:** 和她一起共度的夏日祭 = **A Summer Together With Her** (カノジョとふたりですごす夏). Read: Sayaka 18–19
    (fireflies, her late grandmother, a rainbow firefly) and Aki 11–16 (matchmaking shrine, matching charms,
    fireworks, the lost charm). File: `summer_fest.txt`.
  - Its first 10 minutes are Sayaka's **main chapters 43–44** (spa resort), not an event; they were checked and
    skipped.
  - `research/tw_ama_events.py` adds these as the events' `parts`: Seaside 71 and Summer 8 (scenarios
    `530011…`, `530012…`). Test: `test_ama_stream_event_episodes`; tests: 32 pass.
  - **Event mechanics seen:**
    - Seaside is a slot machine: 「大中獎了!!」 awards the next episode, and the 「特殊衣裝」 reel shows the outfit wearer.
    - Per-heroine unlocks run from 350 pt (episode 7) to 1500 pt (episode 20).
    - Ranking rewards: pictures of each heroine (1000–10000 pt), 祕聞點數 (secret points) and Love Crystals, with a
      special illustration at 15000 pt.
  - All 34 streams are scanned. The rest of A機's footage is Sayaka's main route (chapters 1–50) and Sayaka secret story 1, which is already in the mock.
- **Low-quality copies** of Lu Dae's (non-Mitsuka), Kwun Tang Kwai's and 開膛手傑克's route videos are downloading to
  `vid/low/` for a popup and title scan. `get.sh` uses `--js-runtimes node -N 4`; without node, YouTube throttles
  downloads to ~10 KB/s.

**Popup scan of 222 route videos (2026-10-14, `hunt4/popups_low.txt`).** These are low-quality copies in `vid/low/`,
checked with `popups.py --any`, which doesn't depend on the video's framing. 24 videos hit. Note that the detector
also fires on other red-over-green dialogs (route-end, limited-picture and shop popups).
- The hits worth a closer look were downloaded at 720p to `vid/hits/`, with contact sheets in `vid/hits/all_*.png`:
  - Kwun Tang Kwai's Misaki chapters 25, 27 and 30, where Misaki's secret stories 4–6 would be.
  - 11 of 開膛手傑克's Aki-route videos.
- **開膛手傑克 owns Aki secret stories 4–7.** Memory in `6LHI7WZDDsg` lists 「秘聞4：強烈又堅定的決心」, 「秘聞5：某年的夏日祭典」,
  「秘聞6：好想撒嬌」 and 「秘聞7：選擇女性的原因」.
  - The scene after 1:55 (Aki's dream is to meet the mother who abandoned her) is told from the hero's side. It may be
    main chapter text rather than secret story 7; check before importing (`vid/tiles2/j6_*.png`).
- `xsdtRCDwqR0` shows Edo-event gacha pulls (Ren 5–6, Misaki 2–3, Aki 6–8, Mitsuka 5, Aoi 5, Sayaka 6), so the Edo
  episodes may also be in 開膛手傑克's other videos.
- ~~Next: tile the remaining hit videos~~ **done 2026-10-15:** the only new text was Aki's Edo episode 1 (below); the
  Kwun Tang Kwai videos have no secret stories (see "Last checks").

**Mitsuka text removed (2026-10-15).** Mitsuka is a high-school student, and her secret stories and her Seaside
Cinderella / Summer Festival episodes are romance scenes. The translations made in the eighth pass (secret stories
1–10, Seaside 1 and 4–20, Summer 1) were deleted from `server/data/scenarios/` and `research/hunt4/`, and the importers
skip her. Her stories stay listed in the mock without text. The source videos are noted above (Lu Dae, A機 #10–#29) in
case the project ever wants them handled differently. **Secret stories with text: 23 of 56.**

**Aki's Edo episode 1 (2026-10-15).** 開膛手傑克's `rQVWlGwCuN0` plays 「大江戸純愛物語 1話」 for Aki: a period-drama film
shoot at the Edo theme park "Moonlight Edo Village", with Aki as a lady samurai. `research/tw_edo_aki.py` and
`hunt4/edo_aki.txt` hold it; it's scenario `530008020101`, added to the Edo event's parts. The recording returns to the
list after 43 seconds, so the episode may be incomplete. 開膛手傑克's other 32 videos (fetched at 480p to `vid/jack/`
and title-scanned) have no secret-story readings: they bought Aki 4–7 but never filmed them being read.

**Last checks of the eighth pass (2026-10-15), all negative:**
- Kwun Tang Kwai's Misaki chapters 25–30 (`vid/kwun/`, title-scanned): Misaki secret stories 4–6 were not bought;
  the chapters run straight on.
- Dante Chow's full channel (29 clips): only Misaki secret stories 1–3, which the mock already has.
- New searches (Seaside Cinderella, Edo event, per-heroine 秘聞): nothing beyond the channels already covered.
- **The Taiwan-edition YouTube footage is now exhausted** for secret stories and events. Remaining leads need other
  sources: the REBIRTH Discord, 八雲 / Mobage, or Bahamut and Facebook screenshots.

**Other sources checked (2026-10-15), all negative:**
- **Bilibili:** yt-dlp's search gets HTTP 412, so `research/hunt4/bili/bsearch.py` calls the web search API with a
  buvid cookie. Ten queries (女子宿舍禁爱令 / 禁愛令, My Sweet Roomies, ファミーユ, うちカノ …; `bili/b.tsv`): only two
  promo videos and an OP, no gameplay.
- **Taiwanese web:** PTT, Dcard and Plurk searches found nothing about the game. The official Facebook page
  (`facebook.com/Legame.famille`, still up, 11K followers) needs a login for its timeline, even through the public
  page plugin, and the Wayback Machine has no captures of it (checked again once it was back up). **A lead:** with a Facebook login, its
  2015–2017 posts (event and gacha announcements, likely with outfit and event pictures) could be read. Bahamut GNN's shutdown article (sn=188855) lists no events.
- **Other editions:** TapTap's Korean, Thai and other listings are mirrors of the Japanese app; no Korean or Thai
  edition existed.
- **Every outbound link on the Bahamut board** (75, decoded from `ref.gamer.com.tw/redir.php`): the 7 Google Drive
  folders are the wardrobe dump already used; the YouTube playlists and channels are all known. The one unlisted
  channel (`UCLbaiLPFdz0tR9x442_6onQ`, 639 videos, list in `hunt4/chan_UCLbai.tsv`) is 彤's, whose secret stories
  (Aki 1–3, Sayaka 1–10) and Edo videos are already in the mock.
- **Japanese blogs:** the only new hit was voice actress Yasuno Kiyono's 2014 Ameblo post about playing Aki (no
  story text).

**What's left (updated 2026-10-15)**
1. **Text of 33 of the 56 secret stories is missing**, plus Misaki 7–10. Aki 1–3, Aoi 1, Sayaka 1–10 and Misaki 1–3 have text (seventh pass); Ren 1–5 and Aoi 2 (eighth pass). Missing: Ren 6–10, Aki 4–10, Aoi 3–10, Misaki 4–6, and Mitsuka 1–10 (not translated: high-school character).
2. **Event Story text:** the Edo event's preview and Sayaka's Edo episode 1 (seventh pass), Aki's Edo episode 1, the hot-spring event's Sayaka and Aoi routes (40 episodes), the Christmas event's Ren route (20 episodes), Seaside Cinderella (71 episodes, five heroines) and A Summer Together With Her (8 episodes) (eighth pass); event pictures: 3.
3. **The full dress-up pool:** 145 outfits are in the mock (125 from the Taiwan wardrobe, 2026-10-14), against
   about 180 in the game (4–7 missing per heroine). The Face items have art since 2026-10-14 (lifted from story
   pictures), and 10 of the board's 13 backgrounds were found in story pictures (2026-10-14/15). Still unknown:
   Sayaka's room and the morning balcony (in no story picture), the English item names and the gacha rates.
4. **Official English wording** of 57 chat lines, and of the Aoi 5 and Misaki 1–6 secret-story titles
   (fan-sourced for now).
5. **Leads that need an account or a contact:**
   - The "My Sweet Roomies REBIRTH" Discord (invite `ykSUTeVAyy`, about 1,640 members), a fan remake that uses the
     original English dialogue.
   - 八雲 of the Mobage うちカノ circle, who screenshotted the うちカノ events. Reaching him needs a Mobage login.
   - The Taiwan edition's Facebook page (`facebook.com/Legame.famille`): its timeline needs a Facebook login.
6. **Footage leads** (the Taiwan-edition YouTube footage is exhausted as of 2026-10-15; these are English-edition leads):
   - ~~SandDistress #135–#139, for Ren 9~~ **found 2026-10-14:** "Secret Story 9 - A Decision To Be Made" (#138
     `08ymv02llBs` 2:38, behind the next-story popup; `english_footage/ren9/`). Official titles: 50 of 56.
   - Aoi 5 (checked 2026-10-14): SandDistress #25 (`aWaXVqbe8cM` 1:38) finishes Chapter 30 and gets only the EXP and
     "next story is Chapter 31" popups, no secret-story popup (`english_footage/aoi5/`). So Aoi 5 doesn't unlock at the
     end of Chapter 30 as the mock assumes; its real unlock point and English title are still unknown.
   - A frame-by-frame look at SandDistress and Yu_Tsugami, for gacha results and the Clothes tab.
   - ~~Secret stories read after the purchase popups~~ **checked 2026-10-14, none:** Yu_Tsugami buys 16 of them on
     camera ("Get it NOW!", crystals 11 → 6) and SandDistress mostly lacks the crystals, but **neither reads them
     on camera**. The frames 25 s after each popup, the next part of each video and Yu's short parts (28–31, 44, 47,
     54, 61) are all main story (`english_footage/popgrab/`, `popsheet*.jpg`, `yu_secret/*.ocr.json`; the
     downloaded clips were deleted). This fits the Reddit note that "the extra stories" were uploaded and removed.
   - ~~SandDistress #59 "Thanksgiving Special" (`ADnDUKF-Xr0`)~~ **checked 2026-10-15:** just Sayaka's main route
     at 35.4% (chapter 14, already in the mock); "Thanksgiving" is only the upload's title.

## 5. Development notes

Everything needed is in this folder. Nothing depends on a temp folder.

1. **Requirements:**
   - Python 3.14 with `flask`, `cryptography`, `rapidocr-onnxruntime`, `psutil`, `numpy`
   - `yt-dlp` (`pip install -U --pre "yt-dlp[default]"`; older builds fail with "The page needs to be reloaded")
   - ffmpeg on PATH; Node.js (yt-dlp's JS runtime)
2. **The OCR is complete** (2026-10-10). To redo a chapter, move its file out of
   `ocr/out/` and run `bash ocr/queue.sh <Heroine>`. That skips everything still in `ocr/out/`
   and recompiles the route. Run long jobs detached (`nohup bash ocr/queue.sh … &`), because
   long background jobs can get stopped. After each route, check the lines per
   chapter (see the sanity check in section 4).
   - **Never edit `ocr/run_one.sh` while a queue is running:** bash reads scripts as it goes, and
     an edit mid-run broke the copies already in progress.
   - **Disk cleanup (2026-10-13):** the downloaded YouTube clips in `research/ocr/hunt/*.mp4` (1.25 GB, all already
     processed), old sessions' temp folders and pip's cache were deleted. The frame caches (`ocr/hunt/sb`, `pops/`,
     `ss/`) were kept. A clip that's needed again has to be re-downloaded with yt-dlp.
3. **CPU cap (to keep the PC usable):** since 2026-10-09 it's 3 workers pinned to 12 of 20
   logical cores, about 60%. That's the default in `ocr/queue.sh`; override it with
   `OCR_WORKERS`/`OCR_CORES`. It was 2 workers on 8 cores before. RapidOCR ignores thread
   settings, so the cap is CPU affinity (psutil). Full speed maxes the CPU, so don't run it
   uncapped.
   - **Windows proxy:** Windows has a system proxy set to `127.0.0.1:8080`, left over from
     mitmproxy/Fiddler, and yt-dlp picks it up. `run_one.sh` passes `--proxy ""` so downloads
     ignore it.
4. **YouTube sign-in:** if YouTube asks to "confirm you're not a bot", put a Netscape
   `cookies.txt` next to the scripts. Some videos only work without cookies, so the scripts try
   both. Delete the cookies file afterwards.
5. **Skipped lines (done 2026-10-12):** to redo, run `python find_gaps.py Aoi Aki Ren Mitsuka
   Sayaka Misaki`, then `python gap_rescan.py` (skips videos already in `ocr/gaps/`), review the
   candidates, edit `ACCEPT` in `build_gap_patches.py`, run it, then `python compile_story.py <routes>`.
   **Watch for duplicate names** in `chapter_videos.txt` / `cards/list.txt`; a second upload of
   one chapter needs its own name (like `Sayaka_C48b`) and a `CONTINUATION` entry.
   **Proofreading fixes (2026-10-15)** are keyed by story id and line index, so a change that adds or drops lines in
   a part (a new patch, a merge rule) shifts them: `compile_story.py` prints "typo fix not applied" for any that no
   longer match; correct the line index in the matching `research/pp*.tsv` row and run `python proofread_choose.py
   write`. Don't regenerate `pp.tsv` … `pp5.tsv` or `pp_mitsuka.tsv`: `proofread_overrides.py` refers to their row
   numbers. Whole-line fixes (`line_fixes.json`, from the frame check) only apply while the line still reads exactly
   as it did; "line fix not applied" means it changed, so re-check that line. Order in `compile_story.py`: patches →
   `TYPO_FIXES` → `after_fixes` → `LINE_FIXES`. After `fc2_apply.py` always run `fc2_quotes.py`, then recompile
   with every route: `python compile_story.py Ren Aki Mitsuka Aoi Sayaka Misaki secret` (run with no routes it
   writes an empty `line_src.json`).
6. **Next session (as of 2026-10-12 night), in order:**
   1. ~~Cut the other heroines' outfits~~ **done 2026-10-13** (`mega_outfits.py`, 19 outfits). Optional: the
      `Outfits/Unsorted/` cards (more events, mostly Aki).
   2. ~~Rebuild Aoi's default portrait~~ **done 2026-10-13** (`aoi_stand_from_store.py`). Rebuild Aoi's default portrait from the 1440×2560 Vietnamese store screenshot (`Random/screen-2.png`), as
      `aki_stand_from_store.py` did Aki's.
   3. ~~Add the CGs to the Photo window~~ **done 2026-10-13** (`cg_photos.py`, 10 pictures; see section 4
      "Mega Assets pack").
   4. ~~Event cards as Event Story banners~~ **done 2026-10-13** (`event_banners.py`: 9 new events, 14 banners).
   5. ~~Font, sound effects, title screen~~ **done 2026-10-13** (see section 4 "Mega Assets pack").
   **All of step 6 is done.** Possible next steps:
   - ~~more of Aki's outfits from the `Unsorted/` home screenshots~~ **done 2026-10-13** (`aki_home_outfits.py`);
   - ~~the opening BGM on the title screen~~ **done 2026-10-13**, with the title rebuilt from its `.ccbi` layout;
   - ~~WebP for the story pictures~~ **ruled out:** the app crashes on JPEG data under a `.png` name (section 4
     "Story pictures"), so non-PNG data is very likely to crash it too.
   - (the APK has only three `.ccbi` scenes, title, notice and debug, so nothing more to read there; the rest of
     the UI was web pages or story scripts served by the game's servers)
   - converting `img/resource` (7.3 GB of PNG story pictures) to WebP to save ~5 GB.
   Open leads: a reply from 八雲 (message text in `research/hunt2/yakumo/msg.txt`) and the "My Sweet Roomies REBIRTH" Discord.
7. **Taiwan wardrobe (2026-10-13 night – 10-14): steps 1–2 done.**
   1. ~~Finish cutting the Taiwan wardrobe~~ **done:** `cd research && python tw_wardrobe.py cut` (resumable: skips
      cut-outs already in `hunt2/merge/tw/`; 144 in all, ~1 min each at the CPU cap; check how many are done with
      `ls hunt2/merge/tw | wc -l`). Run it detached for long stretches (long background jobs can get stopped).
      **2026-10-14:** the first 91 cut-outs had dark hair ~30 levels too bright under a tinted band at the crown
      (the old bar fit assumed 50% opacity, fitted on sky only). New fit (`python tw_wardrobe.py fitbar` ->
      `hunt3/drive/bar_fit.npz`): opacity K = 0.346 in all channels, a red tint varying left to right, a row
      gradient averaged over all 144 shots; hair under the bar now matches below it within ~2 levels. All 144
      re-cut with it (the old ones were deleted).
   2. ~~Look through the cut-outs on contact sheets~~ **done** (section 3 "Taiwan-edition wardrobe"): drop outfits the mock already has (the Mega Assets card cuts are
      worse than these real home-screen shots, so prefer the TW ones and retire the card cuts), fix leftovers, give
      them descriptive names, copy to `img/heroine/costume/<id>_tw<nn>.png` and add them to `COSTUMES` in `app.py`.
      **Done 2026-10-14 (picking):** `research/tw_costumes.py` holds the 125 picks and names (104 new outfits; 21
      replace older cut-outs of the same outfit, mostly Mega Assets cards) and copies them to
      `costume/<id>_tw<nn>.png`; `COSTUMES` in `app.py` points at them. Re-cut finished, pictures copied, 26 tests pass (2026-10-14).
      Sayaka's "Yukata" renamed "Haori"; Aoi TW:27 / Mitsuka TW:28 are the wedding dresses (user agreed).
      Note: the cut's Python process grew to ~13 GB RAM over the run; restart it every ~50 shots next time.
   3. **Done 2026-10-14:** ~~the Edo event gacha video~~ (Sayaka's Edo episode 1, section 4 "Seventh pass");
      ~~the long Aoi/Mitsuka/Sayaka playthroughs~~ in `hunt3/tw_videos.tsv` (59 videos, 11 h): **main story only**.
      Their storyboards (`hunt3/tw_long/sb/`, one thumbnail every ~11 s; Mitsuka's are edited compilations at ~40 s)
      were scanned for the Memory window, the only way into a secret story: the 16 window-like hits were the
      next-chapter popup, the out-of-story-tickets popup (5 crystals to refill) and "To Be Continued" cards
      (full frames in `hunt3/tw_long/grab/`; `window_hits.json`, `hits.png`). 彤's other uploads were checked (nothing
      new). ~~A Dress Up Face tab~~ **done 2026-10-14**, with art lifted from the story pictures
      (section 3, "Dress Up > Face").
8. **Eighth pass (TW secret stories and events, 2026-10-14 – 10-15): done.** The text lives in `research/hunt4/*.txt`
   ("== <key> <TW title> (<source>)", then "speaker | line"); fix a line there and re-run its importer from `research/`:
   `tw_secret_stories2.py` (Ren 1–5, Aoi 2), `tw_hotspring.py`, `tw_xmas.py`, `tw_ama_events.py` (Seaside, Summer),
   `tw_edo_aki.py`. Each rewrites its `scenarios/<id>/transcript.json` files and its entries in `secret_stories.json` /
   `event_stories.json`, so they can be run in any order. **Never add Mitsuka's romance text** (high-school
   character); the importers skip her. The downloaded videos in `research/hunt4/vid/` (A機 streams, 開膛手傑克,
   Kwun Tang Kwai, the 222 low-quality route videos) were **deleted 2026-10-15** (with the earlier passes' leftover
   downloads in `hunt3/tw_secret`, `hunt3/tw_long` and `ocr/hunt/*`; ~4.4 GB). The tile sheets the text was read
   from are kept; any video can be re-downloaded with yt-dlp.
9. **Check the server:** `cd server && python -m pytest test_server.py` (32 tests, 2026-10-15).

## 5a. Offline APK (2026-10-15)

`offline/MySweetRoomies_offline.apk` plays the whole mock with no PC and no network. It installs as
`jp.mfapps.novel.roomies.offline` ("Roomies! Offline"), next to the original.

- **How:** the decoded 1.4.6 APK (`offline/decoded`, apktool 2.10.0) with `site_url`/`resource_url` in
  `app.json` set to `http://127.0.0.1:18765`, and a content provider `mock.offline.Boot` that starts a small HTTP
  server in the app process before the game's first request. The server (`offline/java/mock/offline`:
  `HttpServer.java` + `Game.java`, compiled to `classes2.dex`) is a port of `app.py`'s per-player rules with
  SQLite; everything fixed (story list and unlock order, transcripts in engine format, picture lists, outfit, face
  and background tables) is exported from `app.py` itself by `offline/export_data.py` into `assets/server`.
- **Pictures:** all PNGs under `img/` are stored as WebP q90 (`export_static.py`, 7.6 GB to 949 MB). The WebView
  gets WebP as is; the story engine (PNG only) gets a PNG made once and cached (60–200 ms per picture).
- **Signing:** `offline/offline.keystore` (password `roomies`). The AES key comes from the APK's own signature at run
  time, so the header crypto works with any key.
- **Rebuild after server changes:** `cd offline && python export_data.py && python export_static.py && python
  build_apk.py`. Rule changes in `app.py` must also be ported to `Game.java`.
- **Installing on real hardware:** the original targets API 21, which newer Android refuses to install ("not
  compatible" — API 14 blocks `targetSdk < 23`, API 15/16 block `< 24`), so `build_apk.py` bumps
  `targetSdkVersion` to 24 (the lowest that installs everywhere while keeping cleartext HTTP on by default and not
  needing explicit `android:exported`), adds `usesCleartextTraffic="true"`
  for the localhost mock, and drops the permissions an offline build never uses — the ones Play Protect flags on a
  sideloaded repack (`INTERACT_ACROSS_USERS_FULL`, `SYSTEM_ALERT_WINDOW`, `WRITE_SETTINGS`, `GET_TASKS`, Play
  Billing, C2DM push, `GET_ACCOUNTS`, `DISABLE_KEYGUARD`, `ACCESS_WIFI_STATE`). The generic unknown-source warning
  still needs a one-time "install anyway". `lib/armeabi/libotome.so` is 32-bit-only, so a 64-bit-only phone
  (e.g. Pixel 7+) still can't run it — there's no source to rebuild it for arm64, so it's being rewritten (see "64-bit rebuild" below).
- **64-bit rebuild (started 2026-11-10, paused 2026-11-11 after phase 1; not yet linked or tested on a device):** a new `libotome.so` for
  `arm64-v8a`, so the APK runs on 64-bit-only phones (Snapdragon 8 Elite class, Pixel 7+) that can't run the
  32-bit original at all. Working notes, rules and per-subsystem status: `offline/native64/REWRITE.md`.
  - **What the original is:** `lib/armeabi/libotome.so` (5.9 MB, the only native library) is **cocos2d-x 2.2.6**
    (build paths `cocos2d-x-2.2.6/projects/cocos2dx-MaidOtome/proj.roomies`, the studio's shared "MaidOtome" novel
    engine) plus the game's own C++ (~700 functions in ~100 classes: scene manager, story parser, canvas and
    character rendering, text window, title/option screens). The dynamic symbol table kept every C++ name and
    signature, which is what makes a faithful rewrite possible. Java does all networking and downloads
    (`ResourceLoader` fetches the story JSON and pictures to files) and plays sound (`AudioProcessor`); the native
    side reads local files and draws.
  - **How it's rebuilt:** stock cocos2d-x 2.2.6 compiled for arm64 with NDK 27.2 through CMake + Ninja
    (`offline/native64/CMakeLists.txt`; ndk-build can't handle the space in the project path). Only the pieces the
    game uses: cocos2dx core, CocosDenshion, and the CCBReader / CCControlExtension / CCScrollView extensions;
    libpng 1.6.44 from source. JPEG/TIFF/WebP are compiled out (every asset is PNG, and the 2014 prebuilt image
    libraries have no arm64 build). The core links whole-archive, because the engine's JNI entry points are only
    called from Java and a normal static link drops them. `tools/fetch_deps.py` downloads cocos2d-x and libpng and
    reapplies the one engine patch (CCImage PNG-only), so a fresh clone can rebuild.
  - **The game code:** decompiled with Ghidra 12.1.4 headless (`native64/re/`, kept local and out of git), with
    the PIC string loads resolved to their literals (`re/resolve_strings.py`; Ghidra loads the .so at base
    0x10000) and every symbol grouped per class as the interface contract (`re/signatures.txt`). Each class is
    rewritten as readable C++11 in `native64/src/` with the original class and method names, so the unchanged
    Java side and the `.ccbi` layouts bind to it as before. It's done in five subsystems:
    - **story-core (done):** story / scenario / offset JSON data (picojson), every enum with the original's values
      (read from its static initialisers), `SceneDataParser`, `SceneManager`. Event cycle: Setup, then per line
      PreRender, PreEffect and MainRender (from the second line on, PostEffect of the previous line first), then
      Finish; `link_scene` "1" advances by itself after 0.25 s. Default auto-play delay 4.25 s.
    - **platform (done):** all 27 game JNI entry points, checked against the smali; `AppDelegate`; config
      (`/json/app.json` with the `product` and `product/android` overlays); preferences; `SoundEngine` through Java
      `AudioProcessor`; `MediaConnector`, `ResourceManager`, `TextureManager`, `SceneConnector`.
    - **render, text, ui (next; paused 2026-11-11 before any files were written):** canvas, characters,
      backgrounds, effects, filters and shaders, the story scene; the text window and backlog; title / notice /
      debug screens, options, menus and widgets. Each can start straight from `REWRITE.md` (the phase-1 Status
      sections list the APIs they code against and the headers phase 1 expects from them: `FilterData.h`,
      `BaseFilter.h`, `BeforeScene.h`, `AfterScene.h`, `ColorString.h`).
  - **Checks:** `tools/check.py` syntax-checks sources with the real build flags; `tools/check_exports.py`
    compares the new library's `Java_*` exports with the original's 44 (all 17 cocos2d-x ones present; the game's
    27 need the finished link).
  - **Deliberate changes:** where the original crashed on bad input (wrong-typed JSON, missing offset entries,
    null Java strings, failed JNI lookups) the rewrite skips or uses defaults. The four image filters call their
    Java methods as boolean, where the original read them as float (wrong JNI type; nothing reads the result).
  - **Packaging:** `build_apk.py --arm64` adds the build as `lib/arm64-v8a/libotome.so` next to the original
    `armeabi` one; without the flag the APK is unchanged. Phones that can still run 32-bit code will also pick the
    64-bit build (Android prefers the primary ABI). The library links with 16 KB page alignment for Android 15+.
  - **Still to do:** write render/text/ui, full link and export check, `--arm64` APK build, then testing on a
    device (held until the user says so).
- **Loading-screen crash (fixed):** `AuthInfo.readDeviceId` called `TelephonyManager.getDeviceId()`, which throws
  `SecurityException` on Android 10+ for a non-privileged app; it crashed on the WebView's first request, so the
  app sat on the loading screen forever on real phones (the MuMu emulator doesn't enforce the restriction, which
  is why it was missed). `build_apk.py` patches that call in `AuthInfo.smali` to `const/4 v1, 0x0`, so the method's
  existing Build-based fallback id is used; no telephony permission is needed and it runs on every phone. Confirmed
  on a real moto g stylus 5G (Android 13): installs with no granted permissions, title screen and Prologue play.
- **Stories failing to load / "Failed Loading" (fixed 2026-10-16):** on some devices (confirmed on a Galaxy Tab
  S9 FE, Android 16) the native engine's HTTP client (Volley) sends the request line in **absolute-form**
  (`GET http://127.0.0.1:18765/my/ HTTP/1.1`) rather than origin-form (`GET /my/`). The in-app `HttpServer` was
  using the whole target as the path, so every route missed and the request fell through to the SPA index page;
  the engine's `ProxyJson.parse` then failed on the HTML, which the mypage loader swallows (home still renders
  from the WebView) but the adventure loader reports as "Failed Loading", and in a `--debug` build the engine's
  own `AppLog.loge` even crashes formatting the HTML (a stray `%`). `HttpServer.parse` now strips the scheme and
  authority from an absolute-form target (RFC 7230 §5.3.2), so mypage and all story loading work. The static
  requests always worked because the WebView sends them origin-form. Confirmed: stories load and play on the tablet.
- **Checked on MuMu (Android 12):** title, Prologue, home, touch scene, Dress Up, name change. Android shows its
  usual "built for an older version" and permission prompts on first launch, as for the original.
- **Name:** More > Name (and a one-time prompt after the Prologue) opens an in-page box (the app's WebView has no
  `prompt()`); both servers put the chosen name in place of William in the speaker box and story text.

## 5b. GitHub repository (2026-10-15)

- **Where:** https://github.com/BlackrUSter4/mysweetroomies (public). The offline APK is the asset of release
  **v1.0** (`MySweetRoomies_offline.apk`, 955 MB).
- **License:** AGPL-3.0 (`LICENSE`) for the project's own code and documentation. `README.md` states that the
  game's content (story text, characters, art, music, voices, names, logos) belongs to its owners and isn't covered,
  and that the project isn't affiliated with the rights holders.
- **What's in it** (`.gitignore`): `README.md`, `DOCUMENTATION.md`, `LICENSE`, `server/` (code, tests, data, story
  transcripts, and the static files except the story pictures) and `offline/` (the in-app server's Java source and
  the three build scripts), about 360 MB.
- **What's left out:** `research/` (videos, OCR output and frame caches, tens of GB), the original APK and anything
  built from it, `offline/decoded`, `offline/build`, `offline/tools`, the signing keystore, the story pictures
  (`server/data/static/img/resource`, 7.3 GB; they're inside the APK), the local player database and logs. Without
  the story pictures, stories play on a plain background and the tests that check pictures fail.
- **Publishing an update:** commit and `git push`; for a new APK, rebuild (section 5a) and attach it to a new
  release. A rebuilt APK is signed with a new key unless `offline/offline.keystore` is kept, and Android won't
  install it over the released one without uninstalling first.

## 6. Still unknown or guessed

| Item | Status | How it could be found |
|---|---|---|
| Transcript JSON nesting | **confirmed** 2026-10-09 (section 2a) | — |
| Backgrounds and character sprites in stories | **replaced by recording frames** 2026-10-10 (section 4 "Story pictures"); original art still lost | original `img/resource/bg` files, if they ever turn up |
| Misaki's unlock condition | **settled** 2026-10-11: after the other five routes (DevilGh0stz `mIkmTpGmXxM`); the mock does this | — |
| How many Memory rows are replayable without the Memory Pass | mock: the latest 2 per heroine (guess from `FNCwWtZM420` 2:30) | more Memory footage |
| Post-story lines | **all six recorded** 2026-10-10 (section 3 UI) | — |
| Photo window, panel pictures | **recovered** 2026-10-11: three tabs, 16-panel grid, Aoi's four panel pictures and her Epilogue 1 picture (section 4 "Footage hunt"); 10 more limited pictures from the Mega Assets pack 2026-10-13, 3 titled | titles and periods of the rest |
| Epilogue price | **10 crystals in English too** ("Get the Epilogue!!" popup, sixth pass) | — |
| Misaki's Character card text | **found** 2026-10-13 in the Taiwan wardrobe dump (my translation) | official English wording |
| Gacha price, costume pool | **5 crystals a pull** (the JP screen and the English "Machine" screen, 2026-10-12); 10 Machine Points = 1 spin; dress-up tabs Face/Clothes/Background; four English outfits recovered with art (Aki coat, Aoi swimsuit, Sayaka yukata and maid; re-cut 2026-10-12), six more on the gacha banner (too small to use); Ren's six from the Mega Assets pack (2026-10-12), the other heroines' 19 and Aki's 10 + Ren's cat-ear maid from Chinese home screenshots (2026-10-13); rates, item names, Face/Background items unknown | footage of the Clothes tab and gacha results |
| Touch | English touch scenes for Aki, Sayaka, Aoi; **all 82 voiced chat lines** (JP APK clips), 25 with official English (2026-10-12) | English wording of the other 57 |
| Login-streak energy cap | **found**: 5/6/7/8 at 1/2/5/8 straight days; a missed day drops it to 5 (English tutorial `EqwSQBUXVfE` 12:14, predecessor `mtlCHtI8VmM` 0:22); in the mock | — |
| Event Stories | JP event names, dates and format from archived tweets (section 4); no text | event footage |
| Route structure | **found** 2026-10-12: two halves per route, Epilogue 1 after the first half (section 4 "Fifth pass"); in the mock | — |
| Secret Story / Event Story lists | 56 secret stories in the mock, 49 with official English titles, 5 crystals each; text for Aki 1 and Aoi 1 (sixth pass); Event Stories: names only | REBIRTH Discord; more SandDistress/Yu_Tsugami footage |
| Whether the Prologue costs energy | guessed (it does in the mock) | footage of a fresh account |
| 3 chapter titles (Mitsuka 13, Aoi 54, Sayaka 48) | **official English found** 2026-10-12: The Bikini Show / Our Happiness / Brief Respite | — |
| 64-bit-only devices | **in progress** since 2026-11-10: arm64 rewrite of `libotome.so` on cocos2d-x 2.2.6 (section 5a, "64-bit rebuild"; `offline/native64/REWRITE.md`) | — |
| Story text, all routes | **done** 2026-10-10; skipped-line pass done 2026-10-12 (67 lines restored, Sayaka 48 part 2 added); proofreading pass + frame check 2026-10-15 (3,258 lines corrected); full second read against the videos 2026-10-15 (1,414 more + quotes on 2,013); punctuation-only slips remain | — |

## 7. Sources and tools

**Sources**
- APKs: My Sweet Roomies! 1.4.6 (APKPure) and ようこそ！ファミーユへ (APKPure), decompiled with jadx.
- [Official site (Wayback)](http://web.archive.org/web/2016/http://famille.novel.mfapps.jp/):
  heroine profiles and voice samples.
- [Game server 401 capture (Wayback)](http://web.archive.org/web/20170602210754/http://android.roomies.novel.mfapps.jp/).
- [gamebiz: service end](https://gamebiz.jp/news/253513) ·
  [gamebiz: launch](https://gamebiz.jp/news/132876) ·
  [4Gamer](https://www.4gamer.net/games/264/G026427/) ·
  [Famitsu](https://app.famitsu.com/20140627_395416/) ·
  [TapTap](https://www.taptap.io/app/41522) ·
  [applion](https://applion.jp/android/app/jp.mfapps.novel.famille/)
- YouTube:
  - [DevilGh0stz](https://www.youtube.com/channel/UCQ_euda5Neg-gL4xz7CTuvA): every chapter
  - [Jan Antonio](https://www.youtube.com/channel/UCXi699Ngvq_wWcP7uCqCuFA)
  - [The Technical Wizard's World](https://www.youtube.com/channel/UCuBSSXOYiisz_rwVpAxelAg):
    Prologue, shop screens, the Aoi route, closure
  - [HDBarbecho](https://www.youtube.com/channel/UCSF3vFLoJBUR_yJnbY33Hmw)

**Tools in `research/`**

| Script | Does |
|---|---|
| `frames.py` | video → contact sheets |
| `lines.py` | settled dialogue boxes → sheets (how the Prologue was read) |
| `grab.sh`, `sheets.py`, `build_titles.py` | chapter title cards → `chapter_titles.json` |
| `count_parts.sh`, `parts.py`, `compile_parts.py` | loading screens → `chapter_parts.json` |
| `afterload.py` | the frame after each load in long recordings (found the skipped chapter numbers) |
| `story_ocr.py`, `ocr/run_one.sh`, `ocr/queue.sh` | OCR transcription of chapter videos |
| `compile_story.py`, `patches.json` | OCR output → server story files, with hand fixes |
| `aoi_stand_from_store.py` | Aoi's `04_stand.png` + face from the Vietnamese store screenshot (`--install`) |
| `event_banners.py` | Mega Assets event cards → `img/event/<id>.jpg`, 9 new events in `data/event_stories.json` |
| `aki_home_outfits.py` | Aki's 11 outfits from Chinese-edition home screenshots (`--install`); uses `untint_bar.py` (undoes the translucent top bar) |
| `ccbi_dump.py` | reads the APK's CocosBuilder `.ccbi` scene files (layout, timelines); used for the title screen |
| `find_backgrounds.py` / `dress_backgrounds.py` | find the Dress Up backgrounds among the story pictures / write the cleaned ones to `img/heroine/bg/` |
| `tw_wardrobe.py`, `tw_secret_stories.py`, `hunt3/baha/grab_all.py`, `hunt3/tw_secret/tiles.py` | Taiwan-edition pass: wardrobe dump (`classify` / `fitbar` / `cut`), 15 secret stories + Edo preview, Bahamut board mirror, reading tiles |
| `tw_edo_gacha.py` | Sayaka's Edo event episode 1 (16 lines) + event gacha facts, from `HFtNCIJ2i1Q` |
| `tw_secret_stories2.py`, `tw_hotspring.py`, `tw_xmas.py`, `tw_ama_events.py`, `tw_edo_aki.py` | eighth pass: `hunt4/*.txt` transcripts → Ren 1–5 / Aoi 2 secret stories, hot-spring, Christmas, Seaside + Summer, Aki's Edo episode 1 |
| `hunt4/tiles2.py`, `missed.py`, `cliptiles.sh` | dialogue-box tile sheets for reading by eye (new box = >1% of pixels change by >60); audit of boxes the old detector merged; A機 stream clips |
| `hunt4/titles.py`, `popups.py` | part titles after each loading screen (RapidOCR, `--crop`); secret-story purchase popups (`--crop` / `--any`) |
| `hunt4/yt_search.py`, `chan.py`, `desc.py` | YouTube search → tsv; a video's whole channel → `chan_lists.tsv`; descriptions → `desc.json` |
| `location_bgs.py` | story-picture clusters named from the story text → Dress Up backgrounds (`dress_backgrounds.py` installs them) |
| `face_items.py` | Dress Up Face tab: expressions lifted from story pictures (`scan` / `recrop` / `sheet` / `layout` / `build`) |
| `tw_costumes.py` | the 125 picked Taiwan-wardrobe outfits and their names → `img/heroine/costume/<id>_tw<nn>.png`; prints `COSTUMES` |
| `cg_photos.py` | 10 Mega Assets limited pictures → `img/photo/` + `data/photos.json` |
| `mega_outfits.py` | the other heroines' 19 Mega Assets outfit cards → `hunt2/merge/mega/` (`--install` copies to `img/heroine/costume/`) |
| `costume_cutout.py` | the four outfit screenshots (imgur 7CWVK) → `img/heroine/costume/*.png` |
| `photo_pictures.py` | `DBzQdN-tyn4` → `img/photo/*.png` and `photos.json` |
| `ocr/hunt/sb_unpack.py`, `sb_flag.py`, `sb_rare.py`, `grabframe.sh` | YouTube storyboards → thumbnails → non-story / one-off screens → full frames |
| `secret_list.py` | the secret-story readings → `secret_stories.json` |
| `ocr/hunt/popups.py`, `popup_batch.sh`, `secret_sheets.py` | every popup of the 71 神崎LEN videos → `hunt/pops/`; sheets of the unlock popups |
| `ocr/hunt/bubbles.py`, `bubble_batch.sh` | home-screen chat bubbles → `hunt/bubbles/` (read by eye into `touch_lines.json`) |
| `ocr/hunt/sb_touch.py` | storyboard search for short story runs between home screens |
| `wayback/wb.py`, `websearch.py`, `nico/nico_search.py` | Wayback CDX/fetch with retries; batch web search; Nico Nico search |
| `touch_scenes.py` | `_Ve8sVkG1XM` → `touch_scenes.json` and `Touch_*.png` pictures |
| `find_gaps.py` | flag likely skipped lines → `gaps.txt` |
| `proofread.py`, `proofread_propose.py`, `proofread_choose.py`, `proofread_overrides.py` | proofreading pass: suspect words → proposals → reviewed fixes → `typo_fixes.json` (applied by `compile_story.py`) |
| `proofread_mitsuka.py`, `grammar_scan.py`, `rot_decode.py`, `frame_check.py`, `fc_sheets.py`, `fc_apply.py` | Mitsuka's proofreading list; LanguageTool over all lines; upside-down endings; crop each suspect line's dialogue box from its video → review sheets → `line_fixes.json` |
| `full_check.py`, `fc2_sheets.py`, `fc2_pick.py`, `fc2_apply.py`, `fc2_quotes.py` | full second read: every line's dialogue box re-read at 960 px → `full_check/` differences → review list / crop sheets → `line_fixes.json`; quotation marks and `fc2_extra.tsv` |
| `hunt4/bili/bsearch.py` | Bilibili video search via the web API (yt-dlp's search gets HTTP 412) |
| `gap_rescan.py`, `build_gap_patches.py` | re-read only the gap windows → `ocr/gaps/`; reviewed finds → `patches.json` |

YouTube may require sign-in ("confirm you're not a bot"). The scripts accept a Netscape
`cookies.txt` and fall back to anonymous downloads.
