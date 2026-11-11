# libotome.so arm64 rewrite — working notes

Goal: a new `libotome.so` for `arm64-v8a` that the unchanged Java side of the 1.4.6 APK loads, so the offline APK runs
on 64-bit-only phones (Snapdragon 8 Elite class, Pixel 7+). The original library (32-bit ARM, cocos2d-x 2.2.6, no
source) stays in `lib/armeabi/` for 32-bit phones.

## Layout

| Path | What |
|---|---|
| `cocos2d-x-cocos2d-x-2.2.6/` | stock engine source (only `cocos2dx/platform/CCImageCommon_cpp.h` patched: JPEG/TIFF compiled out, `CC_USE_JPEG_TIFF=0`) |
| `libpng-1.6.44/`, `third_party/picojson.h` | libraries the original linked; rapidjson comes from `cocos2d-x.../extensions/CocoStudio/Json/rapidjson` |
| `stubs/` | replacements for engine files we leave out (WebP) |
| `src/` | **the game's own code, rewritten** — one `Class.h` + `Class.cpp` per class, flat directory |
| `re/c/<Class>.c` | Ghidra decompile of the original, one file per class (string literals resolved as `/*"..."*/` comments) |
| `re/signatures.txt` | every game symbol, demangled, grouped by class — **the interface contract** |
| `re/nm_all.txt` | all dynamic symbols of the original (cocos2d-x included) |
| `../decoded/smali/` | the Java side (apktool) — JNI names and Java signatures must match it exactly |
| `tools/check.py` | `python tools/check.py src/Foo.cpp ...` — syntax check with the real build flags |

Build: `cmake -G Ninja -B build -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a
-DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release && cmake --build build`
(NDK 27.2.12479018 under `%LOCALAPPDATA%/Android/Sdk/ndk`). `src/*.cpp` is globbed.

## Rules for the rewrite

1. **Same classes, same public methods.** Class names and method signatures follow `re/signatures.txt` (that's
   what the original's vtables and call sites expect, and what the other subsystems code against). Private helpers
   and member fields are yours to name. Keep inheritance from the cocos2d-x classes the decompile shows (vtable
   calls through `CCLayer`, `CCScene`, `CCObject`...).
2. **Behaviour comes from the decompile.** Read `re/c/<Class>.c`, work out what each function does, and write it as
   clean, readable C++11 against the cocos2d-x 2.2.6 API — not a transliteration of Ghidra's pointer arithmetic.
   Offsets like `*(int *)(this + 0x28)` are fields; give them real names and types. Resolved strings
   (`/*"speaker"*/`) are the literal JSON keys, file names, selectors and log text — keep them exactly.
3. **64-bit clean.** The original is 32-bit: never store pointers in `int`, no `long` == 32-bit assumptions, use
   `intptr_t`/`size_t` where the decompile mixes them. JNI: `jlong` for pointer-sized handles.
4. **JNI must match the Java side exactly.** Exported `Java_...` names and the Java methods C++ calls back
   (`JniHelper::getStaticMethodInfo(..., "jp/mfapps/...", "playBgm", "(Ljava/lang/String;)Z")`) — check every class
   name, method name and signature against `../decoded/smali/`.
5. **No stubs or placeholders** unless the original itself is empty. If something genuinely can't be determined,
   implement the most faithful guess and mark it `// GUESS: <why>`.
6. Comments explain *why*, not what. One short line naming the original is welcome where it helps audits:
   `// orig 001ce668`.
7. Don't edit `cocos2d-x-cocos2d-x-2.2.6/`, `CMakeLists.txt`, or another subsystem's files — note what you need in
   your report instead. Don't install or run anything on phones/emulators.
8. Verify with `python tools/check.py <your files>` until clean (warnings in your own code fixed too).

## Subsystems

| Phase | Subsystem | Classes |
|---|---|---|
| 1 | **story-core** | `SceneData`/`SceneEventListener`/`SceneListenerEventKey` and every `Ex*` data class, `EnumString<...>` + the `Ex*` enums, `SceneDataParser`, `SceneManager`, `TextData`, `LayerData`, `OffsetLayerData`, `FrameAnimationData` |
| 1 | **platform** | JNI exports (`re/c/JNI.c`), `AppDelegate`, `AppConfigData`, `ConfigData`, `EnvConfigData`, `SwitchableConfigData`, `maid::*` (preferences, exceptions, `EngineLog`), `NativeConnector`, `MediaConnector`, `SceneConnector`, `ActiveActionConnector`, `SoundEngine`, `ResourceManager`, `TextureManager`, `DeviceInfo`, `AppTools`, `LogMaker` |
| 2 | **render** | `CanvasLayer`, `PersonRender`, `BackgroundRender`, `EffectRender`, `FilterData`, `BaseFilter` + `NoFilter`/`GrayScaleFilter`/`InvertFilter`/`MosaicFilter`/`SepiaFilter`/`BlurFilter`, `BlurSprite`, `ShaderSprite`, `CCFadeInTo`, `RGBAColorLayer`, `HalfGradientLayer`, `ConversationScene`, `BeforeScene`, `AfterScene`, `IdleScene` |
| 2 | **text** | `TextWindowLayer`, `ColorString`, `TextLogManager`, `TextLogDialog` |
| 2 | **ui** | `StartGameScene`/`StartGameSceneLayer`/`StartGameSceneLoader`, `NoticeScene`/`NoticeSceneLayer`/`NoticeSceneLoader`, `DebugSceneLayer`/`DebugSceneLoader`, `OptionMenuScene`, `OptionsDialog`, `OverlayMenuLayer`, `MenuLayer`, `AppMenuLayer`, `MiSlider`, `MiToast`, `MiDialog`, `MiModalLayer`, `MiScale9Sprite`, `CCControlSliderTouchable`, `EnvTableLayer`/`EnvTableLayerData`, `LoadingSceneLayer`, `TouchLayer` |

`re/c/switchD_*.c` and `re/c/llvm.c` are fragments Ghidra split off (switch bodies, helpers); the address in each
`// <addr>` header says which function they belong to (compare with `re/c/_index.tsv`).

## Status

**2026-11-11: all subsystems written; 81/81 sources pass check.py, the library links, 44/44 JNI exports present.** Not yet run on a device.

(Each subsystem appends a short section here when done: files, deviations from the contract, open questions.)

### story-core (phase 1)

**Files** (`src/`): `EnumString.h` (template, header-only) · `ExEnums.h/.cpp` (all `Ex*` enums + `SceneEventType` +
`AvailableScene`, and the global tables `ExScenarioFgSizeNames` … `SceneEventTypeNames`, `AvailableSceneNames`) ·
`ExScenarioData.h/.cpp` (also `ExScenarioDataScene`, `…SceneFg`, `…SceneFgLayer`, `…Audio`, `…Visual`,
`…VisualFade/Filter/Manpu/Particle`, `ExVisualParticleSpec`) · `ExStoryData.h/.cpp` · `ExOffsetData.h/.cpp` (also
`ExOffsetDataOrderKey`, `ExOffsetDataPerspectiveKey`) · `TextData.h` · `SceneData.h` (also `SoundData`, `ViewData`,
`ViewBackgroundData`, `ViewPersonData`, `ViewEffectData`, `ManpuData`, `FadeData`) · `SceneEventListener.h` (also
`SceneListenerEventKey`) · `LayerData.h/.cpp` (+ `instanceof<T>()`) · `OffsetLayerData.h/.cpp` ·
`FrameAnimationData.h/.cpp` · `SceneDataParser.h/.cpp` · `SceneManager.h/.cpp`. Plain structs have no `.cpp`.

**Extra reverse-engineering output** in `re/c_extra/` (made with `re/scripts/ExportExtra.java`, strings resolved):
the original export skipped global-namespace functions, so `helpers.c` has every global helper of the game range
(`parse*`/`assign*` used by SceneDataParser, but also `scalingNode`, `matchParentWidth*`, `playButtonSe`,
`getTextWindowBackgroundPath`, `getCursorAction`, `setDefaultValue`, … that other subsystems need), `init.c` the
`.init_array` static initialisers and `lambdas.c` the `std::` template code touching our types (`make_shared`
constructors = default field values). The EnumString tables were not in any `.c` file; they were read from the
disassembly of the static initialisers (values below are exact). Ghidra addresses = file vaddr + 0x10000.

**Deviations from the contract / original**
- EnumString tables exist once (`extern` globals) instead of one `static` copy per translation unit. Lookups use
  `toEnum()/toString()`; the maps stay public (`stringToEnum`, `enumToString`) for code that indexes them directly.
- Robustness where the original crashed or threw: JSON values of the wrong type are treated as absent (original read
  picojson's union blindly); invalid JSON gives empty data; a character missing from the offset file is skipped and a
  missing layer offset is (0,0) (original: `map::at` → abort); animated layers without a duration/sequence table get
  empty ones; `getCurrentSceneData()` / `startListenerEvent()` return/pass null instead of throwing from
  `vector::at` before the first scene or for an empty story; `nextWithDelay`/`stopNextWithDelay` skip a null running
  scene; listener callbacks are iterated over a snapshot (they may add/remove listeners re-entrantly).
- `nextWithDelay` uses `CCCallFuncO` (null object) instead of casting `onLinkSceneCallBack(CCObject*)` to a
  `SEL_CallFunc`.
- x/y in the offset file and particle/manpu numbers accept JSON numbers as well as the strings the original expects.
- Lip-sync sequences are appended with a re-implementation of GCC 4.9 libstdc++'s buffered `std::inplace_merge`
  (the original merged unsorted ranges, so the result depends on that exact algorithm).

**GUESS fields** (allocated by the original, never read or written anywhere we found): `ExStoryData::properties`
(map<string,string>), `ViewData::reserved8/reserved24`, `ViewPersonData::unusedName`, `FadeData::reserved` (=2),
`FrameAnimationData::unused`.

**Dependencies on other subsystems** (included by class name, called as in `re/signatures.txt`):
- platform: `ResourceManager::getInstance()`, `slurp(const std::string&)`, `getCachePath(const std::string&)`;
  `MediaConnector::getAudioDurationTimeSecond(const char*)` (float seconds). Both headers exist; check passes.
- render (phase 2): `FilterData.h` must define a copyable `class FilterData` with **public fields
  `ExVisualFilterType type` and `int option`** (SceneData holds it by value and `parseFilter` sets `type` directly,
  as the original did) and `hasFilter()`, `createFilter(const std::string&)` returning a `BaseFilter*`
  (`BaseFilter.h`, a `CCSprite` subclass: `getTexture()`, `getTextureRect()` are used),
  `generateFilteredImage(const std::string&)`, `setOption(const std::string&)`.
- text (phase 2): `ColorString.h` with `static cocos2d::ccColor4B ColorString::toColor(const std::string&)`.
- Until those three headers exist `FrameAnimationData.cpp`, `SceneDataParser.cpp`, `SceneManager.cpp` fail
  `tools/check.py` with "file not found"; with temporary stand-ins (since deleted) all story-core files compiled with
  zero warnings.

**Open questions**: meaning of `FadeData::reserved` (always 2) and `ViewData::reserved8/24`; whether
`ExScenarioDataSceneFgLayer` was a struct holding a vector (as here) or derived from one (only the parser uses it).

### platform (phase 1)

**Files** (`src/`): `main.cpp` (all game JNI exports: `JNI_OnLoad`, `Cocos2dxRenderer.nativeInit`,
`SceneConnector.jni_startScene`, the 25 `AppConfigConnector.jni_*`), `AppDelegate`, `ConfigData`,
`SwitchableConfigData`, `EnvConfigData`, `AppConfigData`, `ResourceManager`, `DeviceInfo`, `TextureManager`,
`MediaConnector`, `SoundEngine`, `SceneConnector`, `ActiveActionConnector`, `NativeConnector`, `AppTools`,
`LogMaker`, and `maid::` classes as `SimplePreferenceData`, `MultiPreferenceData`, `EnvPreferenceData`,
`MaidException`, `IOException`, `JsonException`, `EngineLog` (file names without the namespace). All pass
`tools/check.py` with zero warnings given the other subsystems' headers (temporary `src/_pending/` stand-ins for
`BeforeScene.h`, `AfterScene.h`, `FilterData.h` were used and are deleted).

**Recovered values** (from Ghidra re-runs on static initialisers and lambda bodies the export skipped):
config files `/json/app.json`, `/json/product/app.json`, `/json/product/android/app.json` (build type "product",
platform "android"); KEY_* = their snake_case JSON names; `__STORY_ID__` token, `json/__STORY_ID__.json`,
`json/offset.json`, `json/__STORY_ID___resource.json`, URL defaults `/#/top/support/`, `/#/debug/`, `/#/error/`;
EnvConfigData `common` / `__pointer__`, env-list fields `app_name`, `user_id`, prefix `env.`; SoundEngine listener
name `sound_engine`, volume keys `sound_engine.volume.bgm` / `sound_engine.volume.se`; AppDelegate's IO lambdas =
`AppConfigData::getDouble` / `putDouble(...).commit()` (given to both SoundEngine and SceneManager); animation
interval 1/60; default SoundEngine IO = load returns 1, save no-op.

**Deviations from the original** (all intentional, commented in the code):
- `MediaConnector::blur/sepia/grayscale/invert` call `CallStaticBooleanMethod`; the original used
  `CallStaticFloatMethod` on these `(...)Z` methods and therefore always returned false. Nothing in the game reads
  the result (`FilterData::generateFilteredImage` has no caller).
- ConfigData getters type-check (mismatch -> default) where the original read the union blindly; puts on a root
  that failed to parse start a fresh object instead of dereferencing null; EnvConfigData::merge skips non-object
  pairs; DeviceInfo skips the "/files/"->"/cache/" swap when the writable path has no "/files/".
- JNI lookups that the original did not check (`jni_onSceneEnd`, `jni_clearCache`) are skipped on failure; null
  jstrings from Java become "" instead of aborting in `GetStringUTFChars`; `SceneConnector::_startScene` returns
  when the running scene is not a BeforeScene (original: null dereference).
- `NativeConnector`'s `const char*` getters return a per-method static buffer (the original returned the buffer of
  a destroyed temporary). Return types of its empty methods follow the decompile (void).
- Kept on purpose: `ResourceManager::hasDynamicDocument`/`hasDynamicCache` check each other's path, `rmdirs` stops
  after the first sub-directory, `SimplePreferenceData::foreach` passes the whole store, and
  `EnvPreferenceData::setGlobalString` replaces the whole "common" section.

**GUESS**: enumerator names of `LOG` (`LOGTYPE_NONE/DEBUG/ERROR` = 0/1/2) and `maid::LogLevel`
(`LOG_LEVEL_VERBOSE..ERROR` = 1..5); meaning of `EngineLog::_Log`'s int parameter (bit 0x200 gates output);
`SimplePreferenceData::getInt`'s error text ("] value is not double", shared with getDouble); the unused third
parameter of `TextureManager::addAtlas` (texture file); `convertEnvListJson` treats non-string app_name/user_id as "".

**Dependencies on other subsystems**:
- story-core: `SceneManager::getInstance()`, `addListener(name, shared_ptr<SceneEventListener>)`,
  `removeListener(name)`, `doneListenerEvent(SceneEventType, name)`, `setIO(load, save)`; `SceneEventListener`
  6-function constructor; `SceneData::sound` (`shared_ptr<SoundData>`, fields `bgm/se/voice`);
  `SceneEventType_*` and `AvailableScene` + `AvailableSceneNames.toEnum()` from `ExEnums.h`.
- render (phase 2): `BeforeScene.h` (`static BeforeScene* create()` or any CCScene* return,
  `scheduleNextScene(AvailableScene)`, a `CCScene` subclass so `dynamic_cast` works), `AfterScene.h` (CCScene
  subclass), and `FilterData.h` (pulled in through `SceneData.h`). Until they exist `AppDelegate.cpp`,
  `SceneConnector.cpp` and `SoundEngine.cpp` fail check.py with "file not found".

**Open questions**: whether `NativeConnector::isLoadingScene()` returned a value (its body is empty); whether
`ActiveActionConnector`/`TextureManager`/`SceneConnector` had more virtuals than the destructor (+`init`).

### text (phase 2)

**Files** (`src/`): `ColorString.h/.cpp`, `TextWindowLayer.h/.cpp`, `TextLogManager.h/.cpp`, `TextLogDialog.h/.cpp`,
and `TextHelpers.h/.cpp` - the global helpers of the original TextWindowLayer translation unit, used by nothing
else: `getSpriteScale()`, `getTextUpperBound()`, `getCursorAction(CCPoint&)`, `getTextWindowBackgroundPath(const
std::string*, ExSceneShowTail)`, plus `getAutoPlayCursorAction()` for the symbol-less blink action at 001e2648.
ColorString, TextHelpers, TextWindowLayer, TextLogManager pass `tools/check.py` with zero warnings;
TextLogDialog.cpp passed with a temporary `MiModalLayer.h` stand-in (kept outside `src/`).

**Recovered values** (lambda bodies re-decompiled with Ghidra, static initialisers and literal pools read from the
disassembly): listener names `text_window` (also the auto-play listener) and `text_log_manager`; log file
`<writable path>/textlog.json`, missing file -> `"[]"`; `MAX_LOG_SIZE` 80; atlas `Framework/ui/conversation.plist`
/ `.png`; window frames `serif_box_{you,say,think}.png`, front window `serif_box_front_{you,say,think}.png`, cursor
`piko_{front,say,think}.png`, auto-play cursor `auto_play.png` (x3), `footer.png` when height/width >= 1.7; font
`Application/fonts/07YasashisaGothic.ttf`, text colour (44,44,44) everywhere; backdrop CCLayerColor (204,255,255,255)
640x496; cursor bounce 10 px / 0.6 s, auto-play blink 0.75 s; backlog atlas `Framework/ui/menu_textlog.plist/.png`,
`bg_log.png`, `log_box_bg.png`, `btn_window_close(_active).png`, font `arial` 32, cell 640x260, table 640x750,
fill order BottomUp, touch priorities `kModalLayerPriority - 1/-2` (value -130). Every layout number is commented
or named in the code.

**Behaviour notes**: the front ("sub") window is used when the *previous* scene had `linkScene == Link`; the main
window frame and cursor only change when the window style differs from the previous scene's. A hidden speaker clears
the main name plate, an empty one keeps it; the front window ignores `speakerHidden`. The log appends the current
scene's text at PreRender and saves after every line. `ColorString::toString` prints `#%02d%02d%02d%02d` (decimal,
order a r g b) exactly like the original - it does not round-trip; nothing in the game calls it.

**Deviations from the original** (commented in the code):
- `~TextWindowLayer` also calls `removeAutoPlayChangedListener("text_window")`; the original left `this` in
  SceneManager's auto-play table.
- Null guards where the original crashed: `handleOnMainRender` with no scene/text reports done and returns;
  `removeFirst` on an empty log returns false; `save`/`load` skip an empty std::function; `parse` returns an empty
  log for invalid JSON or a non-array; `inflate` treats non-object entries and non-string `name`/`body` as "".
- `TextLogManager::add` returns `this` (the PreRender lambda chains `add(...)->save()` through the returned
  register, which only works if add returned this); `deflate/inflate/parse` are non-static members as in the original.
- The default save function is a no-op, as in the original (it builds the path and writes nothing).

**GUESS**: none in the code. Return types the mangling does not carry were taken from the register use:
`getXxxText()` -> `const char*` (CCLabelTTF::getString), `onMainRender()`... -> the `SceneEventListener`
handler typedefs, helper actions -> `CCAction*`.

**Dependencies on other subsystems**:
- story-core / platform: `SceneManager` (`addListener`, `removeListener`, `doneListenerEvent`, `isAutoPlay`,
  `addAutoPlayChangedListener`, `removeAutoPlayChangedListener`), `SceneEventListener`, `SceneData`, `TextData`,
  `ResourceManager::isExistFile/slurp`. All present.
- ui: `MiModalLayer.h` - a `CCLayer` subclass with a default constructor, `virtual bool init()`, `virtual void
  dismiss()` (vtable slot 0x200, used by `onCloseButtonClicked`), `void setEnableCloseOnTouchLayer(bool)` and
  `static const int kModalLayerPriority`. TextLogDialog adds `virtual void initUi()` (slot 0x208). Until it exists
  `TextLogDialog.cpp` fails check.py with "file not found".
- Callers: ConversationScene (render) uses `TextWindowLayer::create()`, `setNameText/setBodyText`, and
  `TextLogManager::getInstance()->prepare(load, save)` where the original's load returns `"[]"` and save is empty
  (so the backlog lives in memory only); OverlayMenuLayer (ui) uses `TextLogDialog::create()`.

**Open questions**: whether `kCCTableViewFillBottomUp` (value 1 in the original) really puts the newest line where
players expect it - kept as is; why `getTextUpperBound` subtracts the width (a CCSize(width, width) in the
original) - kept, the layout constants only make sense with it.

### ui-widgets (phase 2)

**Files** (`src/`): `MiModalLayer.h/.cpp`, `MiDialog.h/.cpp`, `MiToast.h/.cpp`, `MiSlider.h/.cpp` (also the
`MiSliderTag` struct), `MiScale9Sprite.h/.cpp` (also the file-local `check9PatchPixel`, a weak global in the
original used only here), `CCControlSliderTouchable.h/.cpp`, `EnvTableLayer.h/.cpp`, `EnvTableLayerData.h`
(struct, no .cpp), `LoadingSceneLayer.h/.cpp`, `TouchLayer.h/.cpp`. All pass `tools/check.py` with zero warnings;
so do their current users (AfterScene, BeforeScene, BackgroundRender, ConversationScene, DebugSceneLayer,
StartGameSceneLayer, TextLogDialog).

**Recovered values** (vtables read from the relocations, literal pools and static initialisers from the
disassembly): virtual order MiModalLayer `dismiss` 0x200, `setDismissCallback` 0x204; MiDialog `initUi` 0x208,
`onCloseClicked` 0x20c; EnvTableLayer `initWithSize` 0x200 + data-source/delegate overrides; TouchLayer is a
`CCLayerColor` (white, alpha 0, default priority 1). `kModalLayerPriority` -130; MiSlider registers at -129, init
defaults position 0 / range 0..1 / width 100, notification name `MiSlider%d` (pre-incremented
`GLOBAL_SLIDER_ID`), images `slidebar_{back,front}_{left,middle}.png`, `slidebar_indicator.png`, child tags 0-2
background, 3-5 foreground, 6 indicator, z 1/2/3. MiToast: `DURATION_LONG` 10, `DURATION_SHORT` 2, font `Arial`
32 x winW/640, z 100, label tag 0 / background tag 1, initial background (243,243,243,200), colour pairs (text /
background) debug (3a87ad,ff)/(d9edf7,c8), info (468804,ff)/(dff00d,c8), warn (c09853,c8)/(fcf8e3,c8), error
(b95994,ff)/(f2dede,c8) - Bootstrap's palette with the original's typos kept. LoadingSceneLayer: frames
`Framework/ui/scene_loading/loading-anime-00..08.png`, 0.111 s per frame, loops forever, scale winW/640. EnvTableLayer:
cell 140 (x winW/640, square), key label `arial` 32 at (20,100) anchor (0,0.5) 600x42, value label `arial` 22 at
(20,80) anchor (0,1) 600x76, separator CCLayerColor (52,52,52,255) 640x2 at y 5, fill order TopDown. Every std::function
default is a no-op lambda (checked in the disassembly for MiModalLayer).

**Deviations from the original** (commented in the code):
- `std::function` members are checked before calling (the original aborted on an empty one): MiModalLayer::dismiss,
  TouchLayer::ccTouchBegan, CCControlSliderTouchable::ccTouchEnded, EnvTableLayer::tableCellTouched.
- MiToast schedules a private `onDurationElapsed(float)` that calls `dismiss()` (the original cast `dismiss()` to a
  `SEL_SCHEDULE`); `init` only adds the toast when there is a running scene, and `show` returns when it was not added
  (the original dereferenced a null scene).
- `MiSlider::setSliderValues` skips the range update for an empty list (original: `at(0)` -> abort); the setters
  return `this` (the mangling does not carry return types; the original left `this` in r0 everywhere and the
  value-returning ones are used for chaining); the 256-byte name buffer is a `std::string`.
- `MiScale9Sprite::get9PatchRect` bounds the left-to-right guide scan by the width (original: by the height) and
  returns an empty rect when the image fails to load; `create` returns null for a missing texture.
- `EnvTableLayer::setTouchEnabled`/`invalidate` null-check the table child; `invalidate` drops the original's no-op
  copy loop over the rows.
- Kept on purpose: MiToast tints the background with (g, g, b) of the requested colour; MiSlider builds the right cap
  from the *middle* image, places it at middle.x + middle.width/2 with anchor (1,0.5), never unregisters its touch
  delegate, and `getSliderRelativePosition` snaps only from 3 values while `setSliderRelativePosition` snaps from 2;
  `MiScale9Sprite::create` passes (pixelsHigh-1, pixelsWide-1) as the frame's width/height and shifts the cap
  insets by +1; TouchLayer::setTouchCallback also enables CCLayer's standard touch registration. MiSlider and
  MiScale9Sprite have no callers in the shipped game; MiDialog has no subclass (OptionsDialog, TextLogDialog,
  OverlayMenuLayer derive from MiModalLayer directly).

**GUESS**: `MiDialog::kDialogPriority` - a `.bss` int no initialiser or function touches, defined as 0.

**Dependencies on other subsystems**: none beyond cocos2d-x/extensions (no `UiHelpers` helper is used by these
classes). Provided to others as `re/signatures.txt` declares them: `MiModalLayer` (default ctor, `init`, virtual
`dismiss`/`setDismissCallback`, `setEnableCloseOnTouchLayer`, `kModalLayerPriority`), `MiToast::debug/warn/...`
+ `DURATION_*`, `LoadingSceneLayer::create`, `TouchLayer::create/setTouchCallback`, `EnvTableLayer::create/
setCallback/addData/clearData/invalidate`, `CCControlSliderTouchable::create/setOnTouchEndListener`.

**Open questions**: whether MiSlider had a `create()` (none in the symbols; the slider is unused, so it is built
with `new` + `init()`); what the original meant by the swapped width/height in `MiScale9Sprite::create`.

### ui-scenes (phase 2)

**Files** (`src/`): `StartGameScene`, `StartGameSceneLayer`, `StartGameSceneLoader`, `NoticeScene`,
`NoticeSceneLayer`, `NoticeSceneLoader`, `DebugSceneLayer`, `DebugSceneLoader`, `OptionMenuScene`, `OptionsDialog`
(also `enum OptionsDialogTag : int`, tags Bgm 0 / Se 1 / TextSpeed 3 / Reset 4), `OverlayMenuLayer`, `MenuLayer`,
`AppMenuLayer` (.h/.cpp each), and `UiHelpers.h/.cpp` (`scalingNode`, `matchParentWidth`,
`matchParentWidthFromCanvasSize`, `playButtonSe`, `playMenuInSE`, `playMenuOutSE`, `setAutoTextValueFromSlider`,
`setDefaultValue`). Not in UiHelpers: `check9PatchPixel` (file-local in MiScale9Sprite.cpp), the TextWindowLayer
helpers (TextHelpers.h), `updateMaxSize`/`resetCurtainAction` (CanvasLayer). All 14 .cpp pass `tools/check.py`
with zero warnings; so do BeforeScene.cpp and ConversationScene.cpp, which include these headers.

**Recovered values** (literal pools, GOT entries, lambda bodies and `.init_array` code read from the disassembly; the
decompile did not show them): ccbi files `Application/{StartGameSceneLayer,NoticeSceneLayer,DebugSceneLayer}.ccbi`
(file-static std::strings), loader names = class names. Title particles `ccbResources/start_game_scene_layer/
{feather,feather_small,kirakira}.plist` at (0.5w, -0.05h), (0.5w, -0.05h), (0.5w, 0.5h) in child 3; title layout
ratios 1.0 (tags 1, 2, 4), 0.85 (11 in 10, 13 in 12), 0.1 (15 in 14, 17 in 16), tag 12 hidden; transitions
`CCTransitionTurnOffTiles` 1.5 s (start, debug) and `CCTransitionFadeTR` 1.5 s (notice -> title) after 2.6 s;
splash BGM `Application/sound/bgm/splash/13_01.ogg` (also replayed when the title's OptionsDialog closes).
Menu-item targets of the debug footer: item 25 -> onPressListButton, item 23 -> onPressCloseButton; title font x
winW/640; debug table 12 rows have value "". OptionMenuScene backdrop (238,238,238,255). OptionsDialog: sheet
`Framework/ui/menu_option.plist/.png`, `bg_setting.png` anchor (0.5,0) at y = 200s (s = winW/640), rows at 490s /
350s / 210s / 85s above it, label x0.9, slider x0.75 at label.y - 0.3 x label height (double), sprites
`sliderTrack.png`/`sliderProgress.png`/`sliderThumb.png`, touch priority kModalLayerPriority-1 (sliders) / -2
(buttons), close item anchor (1.3,1.3) on the frame's top-right corner, touch-end samples
`Framework/sound/bgm/bgm_sample.ogg` (BGM, playBgm) and `Framework/sound/voice/vo_sample.ogg` (SE, playVoice),
text-speed touch-end no-op; text speed value = |1 - (delay - 0.25) / (2 (AUTOPLAY_DELAY - 0.25))|, changes =
|max - value|. OverlayMenuLayer: sheet `Framework/ui/conversation.plist/.png`, tint (61,175,170,204) with
`CCFadeInTo(0.3, 204)`, popup `CCFadeIn(0.3)`, strip `menu_bg.png` at (w/2, h/3), frames btn_log / btn_option /
btn_auto_off->btn_auto / btn_mute_off->btn_mute_on (+ `_active`), padding 10s, toggles x winW/640 and
cascade-opacity; dismiss callback = SoundEngine::play() + SceneManager::resume(). AppMenuLayer: `btn_menu.png` /
`btn_menu_active.png`; MenuLayer corner buttons use a 480-wide design and a 10 px margin. StartGameScene's
volume-save lambda does **not** commit (OptionMenuScene's does).

**Deviations from the original** (commented in the code):
- Handlers with the two-argument signatures of the contract (`onPress*Button(CCObject*, unsigned)`,
  `onBgmChanged(CCControlSlider*)`, `onTitleShowCallback()`) are reached through small private adapters with the
  exact SEL_MenuHandler / SEL_CCControlHandler / SEL_CallFuncN signature instead of the original's member-pointer
  casts (calling through a mismatched member-function type).
- `*SceneLayer::scene()` dynamic_casts the ccbi root and leaves the scene empty when the file does not load.
- OptionMenuScene calls `CCScene::onEnterTransitionDidFinish()`/`onExit()` (the original skipped both).
- `~OverlayMenuLayer` resets SoundEngine's mute listener, which captured the destroyed layer.
- OverlayMenuLayer's toggle handlers return on a non-toggle sender (original: unchecked dynamic_cast).
- Kept on purpose: AppMenuLayer::setAppSetting ignores its first selector (no voice button) and the menu item's
  target is the AppMenuLayer itself (see below); MenuLayer's corner buttons sit winWidth below the screen centre
  (the original's CCSize built from the width twice); hideMenuLayer plays the "in" click and showMenuLayer the
  "out" click; setDefaultValue applies each value twice (once through the slider's handler).

**GUESS**: the meaning of StartGameSceneLayer's child tags 10-17 (named as title buttons); enumerator names of
`OptionsDialogTag` (the original only has the numbers).

**Dependencies on other subsystems** (all headers present; included by class name):
- platform/story-core: `SoundEngine` (prepare/release/play*/stop/volumes/mute API), `AppConfigData`
  (`getDouble`/`putDouble`/`commit`, `getEnv`/`getEnvList`/`switching`, the URL getters), `SceneConnector`
  (`setSceneName`/`setCallbackParameter`), `SceneManager` (`AUTOPLAY_DELAY`, auto-play, `pause`/`resume`),
  `AvailableSceneNames`.
- render: `AfterScene::create()`, `RGBAColorLayer::create()`, `CCFadeInTo::create(float, int)`; ConversationScene
  must pass (voice, menuPopup) to `setAppSetting` and its `onTouchMenuPopup` runs with `this` = the AppMenuLayer
  (as in the original), so it may only use CCNode behaviour through `this` (it does: `addChild`).
- text: `TextLogDialog::create()` + inherited `setDismissCallback`.
- ui-widgets: `MiModalLayer` (init, virtual dismiss/setDismissCallback, setEnableCloseOnTouchLayer,
  kModalLayerPriority), `EnvTableLayer`, `CCControlSliderTouchable`.

**Open questions**: whether the debug/notice/title ccbi really bind nothing through the CCControl resolver (all
return null in the original, so the buttons must be CCMenuItems); why the original never called the base
OptionMenuScene::onExit (looks like an oversight).

### render (phase 2)

**Files** (`src/`, .h + .cpp each): `CanvasLayer`, `PersonRender`, `BackgroundRender`, `EffectRender`, `FilterData`,
`BaseFilter`, `NoFilter`, `GrayScaleFilter`, `InvertFilter`, `MosaicFilter`, `SepiaFilter`, `BlurFilter`,
`ShaderSprite`, `BlurSprite`, `CCFadeInTo`, `RGBAColorLayer`, `HalfGradientLayer`, `ConversationScene`,
`BeforeScene`, `AfterScene`, `IdleScene`. The two global helpers only CanvasLayer uses (`updateMaxSize`,
`resetCurtainAction`) are file-local in CanvasLayer.cpp; no RenderHelpers file was needed. All 21 .cpp pass
`tools/check.py` with zero warnings, as do the phase-1 files that waited on these headers (AppDelegate,
SceneConnector, SoundEngine, FrameAnimationData, SceneDataParser, SceneManager).

**Recovered values** (literal pools, GOT entries, lambda bodies and `.init_array` string statics read from the
disassembly with the NDK's llvm-objdump; Ghidra lost most soft-float arguments):
- Listener names `canvas_layer`, `effect_render` (also the scene-lock owner), `conversation_scene`. EffectRender
  registers its own listener: MainRender/PreRender/PreEffect lambdas forward `scene->view` and report done,
  Setup/PostEffect/Finish only report done. ConversationScene: tap = `next()` only if touch-enabled and the delay
  flag is set (then clears it); `setDelayEnd` is scheduled repeating every 0.3 s; story finish ->
  `replaceScene(AfterScene::create())`; TextLogManager IO = load `"[]"`, save no-op; SoundEngine IO = AppConfigData
  getDouble / putDouble + commit; `setAppSetting(onTouchVoicePlay, onTouchMenuPopup)`; text window at the screen
  centre, anchor (0,0), not ignoring its anchor; canvas default position (w/2, h - canvasH/2).
- Scene timing: BeforeScene -> onNextScene after 0.01 s (start_game and notice both go to `NoticeSceneLayer::scene()`),
  AfterScene -> delayScene after 0.5 s; IdleScene backdrop opaque black.
- Filters: shaders `Framework/shader/Common.vsh` + `GaussianBlur.fsh` / `GrayScale.fsh` / `Invert.fsh` / `Mosaic.fsh`
  / `Sepia.fsh` (std::string statics), attributes a_position 0 / a_texCoord 2; pre-rendered file
  `<path minus 4 chars>-<filter name>-<option>.png`; Java blur radius: option 0 -> 7, option 1 -> 15, else 0;
  BlurSprite shader `Framework/shader/BlurShader.fsh` (not in the APK).
- Layout: PersonRender scale = width / 480, slots x = -w/4 (left), 0, +w/4 (right); mouth "05_mouth" rests on frame 2,
  plays its sequence once, other animated layers loop forever. Background sprite scaled to the layer width, particles
  at (0, 0.75h). Manpu / particles: 480-wide design (manpu 0.3 s fade+rise of 20 px; particle pos =
  (w/2 - s*x, s*y - h/2) in double); end marks `fin.png` / `continued.png` from `Framework/ui/conversation.plist/.png`,
  640-wide design, anchor (0.5,1), park 1 s (40 s px above / one width right), 0.5 s slide + fade in, 1 s hold.
- CanvasLayer: shake soft w/30 0.08 s x1, medium w/20 0.08 s x2, hard w/5 0.08 s x3 (fallback w/10 0.1 s x1);
  camera offset = (content - winW)/2, duration (content - 480)/480 x 3 s, 0.1 s delay, Spawn tag 0; fades read the
  *previous* scene's FadeData in both PreEffect and PostEffect; curtains 1/5/9 (black layers at (-w/2,-h/2)),
  gradient pairs 2-3 / 6-7 / 10-11 (anchor (0.5,1), scale -1 / +1); HalfGradientLayer is a winW x winW square
  (solid top half tag 0, gradient bottom half tag 1).

**Deviations from the original** (commented in the code):
- Robustness: null/failed image, particle and animation loads are skipped (original dereferenced them);
  `createFilter` skips `prepare()` on a null sprite; `addFrameAnimationData` returns on an empty frame list
  (original `vector::at` threw); EffectRender/PersonRender handle null views/persons.
- CanvasLayer reports PreEffect/PostEffect done for a fade of type "none" (on != none); the original never did and
  the story stalled.
- `~EffectRender` removes its "effect_render" listener (the original left it registered with a dangling `this`;
  harmless only because Finish clears all listeners before the next ConversationScene is built).
- BackgroundRender forgets its old emitter after `removeChildByTag(2)` instead of calling `stopSystem()` on the
  released object when the new particle file is missing.
- ShaderSprite::draw sets the vertex attribute pointers (the original drew with whatever the previous draw bound);
  initProgram keeps the slurped shader source alive (original used the buffer of a destroyed temporary).
- Callbacks: `setDelayEnd()` / `delayScene()` are scheduled through private `(float)` adapters, and CanvasLayer's
  `(CCObject*)` callbacks through `CCCallFuncO`, instead of the original's member-pointer casts.
- Dropped no-ops: `removeFromParentAndCleanup(true)` calls on parentless nodes (CanvasLayer/BackgroundRender init,
  the new background node), and the unused camera-name string in onRenderAction.
- Kept on purpose: ConversationScene::init does not call `CCScene::init()`; BlurFilter blurs an already pre-blurred
  file again; only the `upper` gradient curtain gets `stopAllActions()` before a centre fade; particle x mirrored as
  above; `CCFadeInTo` has no copy/reverse of its own.

**GUESS**: `PersonRender`'s second vector (cleared in init/onPreRender, freed in the destructor, never filled) is
typed `std::vector<CCNode*>`; `FilterData::setOption` returns `FilterData&` (the original returned `this`).

**Dependencies on other subsystems** (all headers present at the end; included by class name):
- story-core/platform: `SceneManager` (listeners, `next`, `start`, lock/unlock, `setOnStoryFinishListener`),
  `SceneData`/`ViewData`/`FadeData`/... fields, `ExVisualFilterTypeNames`, `OffsetLayerData::getOffsetPoint`,
  `FrameAnimationData::toCCAnimation`, `ResourceManager::isExistFile/slurp`, `MediaConnector::sepia/...`,
  `SoundEngine::prepare/release/playVoice`, `AppConfigData` paths + getDouble/putDouble, `SceneConnector::
  onScenePrepared/onSceneEnd`.
- text: `TextWindowLayer::create/setNameText/setBodyText`, `TextLogManager::getInstance()->prepare`.
- ui: `AppMenuLayer::create/setAppSetting`, `OverlayMenuLayer::create`, `TouchLayer::create/setTouchCallback`,
  `LoadingSceneLayer::create`, `MiToast::debug/warn` + `DURATION_SHORT`, `NoticeSceneLayer::scene`,
  `DebugSceneLayer::scene`, `OptionMenuScene::create`. ConversationScene::onTouchMenuPopup only calls `addChild`
  through `this`, which is the AppMenuLayer at run time (see ui-scenes).

**Open questions**: whether the original's mirrored particle x (w/2 - s*x) matched how the story JSON was authored;
whether the very large "hard" shake (w/5) was intended; how an abandoned story (no Finish phase) was meant to clean
up EffectRender's listener. `onTouchMenuPopup` runs with an AppMenuLayer as `this` (UB, works because it only
calls the virtual `addChild`); the clean fix would be for AppMenuLayer::setAppSetting to take the target object
(e.g. pass the ConversationScene), which needs a ui-side signature change, so it is left as in the original.
