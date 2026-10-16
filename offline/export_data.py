"""Offline APK, step 1 (2026-10-15): export everything the mock server computes from fixed data, so the Java server
inside the app (offline/java) only has to do the per-player parts.

It imports server/app.py and asks the real code, so the APK always matches the Python server:
  - assets/server/scenarios/<sid>/transcript.json, resource_list.json: the "content" of GET /scenarios/<sid>/...
    (engine format, title card, end mark, picture list) for every story, touch scene, event part and scenario folder
  - assets/server/static.json: the story list with each story's unlock predecessor, the outfit, background, face and
    photo tables, the per-heroine fields of the player state, prices and limits
  - assets/server/index.html: templates/index.html with its two placeholders left for the Java server
  - assets/server/character_offset.json
Pictures are step 2 (export_static.py).
usage: python export_data.py"""
import json
import os
import shutil
import sys

R = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(R, "..", "server"))
import app as S                                                    # noqa: E402

OUT = os.path.join(R, "build", "assets", "server")
HOST = "http://127.0.0.1:8080"


def dump(rel, obj):
    p = os.path.join(OUT, rel)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf8") as f:
        json.dump(obj, f, ensure_ascii=False, separators=(",", ":"))


def main():
    if os.path.isdir(os.path.join(OUT, "scenarios")):
        shutil.rmtree(os.path.join(OUT, "scenarios"))
    c = S.app.test_client()
    sids = ({s["scenario_id"] for s in S.STORIES} | set(S.TOUCH_BY_ID) | S.EVENT_SCENARIOS
            | {d for d in os.listdir(S.SCENARIO_DIR) if os.path.isdir(os.path.join(S.SCENARIO_DIR, d))})
    for sid in sorted(sids):
        for kind in ("transcript", "resource_list"):
            r = c.get(f"/scenarios/{sid}/{kind}", base_url=HOST)
            assert r.status_code == 200, (sid, kind, r.status_code)
            dump(f"scenarios/{sid}/{kind}.json", r.get_json()["content"])
    dump("character_offset.json", S.load_fixture(S.DATA_DIR / "character_offset.json") or {"01": {}})

    v = S.STATIC_VERSION
    heroines = []
    for h in S.HEROINES:
        hid = h["id"]
        heroines.append({
            "base": h,
            "face_url": (f"/static/{v}/img/heroine/{hid}_face.png"
                         if (S.STATIC_DIR / f"img/heroine/{hid}_face.png").is_file() else None),
            "touch_id": S.TOUCH_ID.get(hid),
            "touch_line": S.TOUCH_SCENES.get(hid, {}).get("popup"),
            "chat": [{"text": x["en"], "voice": x["voice"]} for x in S.TOUCH_LINES.get(hid, [])],
            "costume_pool": [S.DEFAULT_COSTUME] + list(S.ALL_COSTUMES.get(hid, {})),
            "face_pool": [{"name": n, "url": f"/static/{v}/img/heroine/face/{S.FACES[hid]['faces'][n]}"
                           if n in S.FACES.get(hid, {}).get("faces", {}) else None} for n in S.FACE_NAMES],
            "faces": list(S.FACES.get(hid, {}).get("faces", {})),
            "popup_url": (f"/static/{v}/img/heroine/{hid}_popup.png"
                          if (S.STATIC_DIR / "img" / "heroine" / f"{hid}_popup.png").is_file() else None),
            # every outfit she can wear -> her picture; "" = no outfit equipped
            "stand": {k: S.stand_url(hid, k) for k in [""] + list(S.ALL_COSTUMES.get(hid, {}))},
            "gacha_pool": list(S.COSTUMES.get(hid, {})),
            "card_art": list(S.CARD_ART.get(hid, {})),
            "main_ids": [s["scenario_id"] for s in S.STORIES if s["heroine_id"] == hid and s["kind"] == "main"],
        })
    overlays = {}
    for h in S.HEROINES:
        hid = h["id"]
        for costume in S.ALL_COSTUMES.get(hid, {}):
            for face in S.FACES.get(hid, {}).get("faces", {}):
                o = S.face_overlay(hid, costume, face)
                if o:
                    overlays[f"{hid}|{costume}|{face}"] = o
    stories = [{**s, "prev": S.previous_story(s)} for s in S.STORIES]
    photos = [{"kind": p["kind"], "entry": S.photo_entry(p),
               "needs": S.story_id(p["heroine_id"], int(p["title"][-1]), 1, arc="02") if p["kind"] == "epilogue" else None}
              for p in S.PHOTOS]
    dump("static.json", {
        "api_version": S.API_VERSION, "static_version": v, "first_story_id": S.FIRST_STORY_ID,
        "story_prefix": S.STORY_PREFIX,
        "ticket_max": S.TICKET_MAX, "special_ticket_max": S.SPECIAL_TICKET_MAX,
        "ticket_regen": S.TICKET_REGEN_SECONDS, "delux_regen": S.DELUX_REGEN_SECONDS,
        "crystals_per_energy": S.CRYSTALS_PER_ENERGY, "item_prices": S.ITEM_PRICES, "pass_days": S.PASS_DAYS,
        "bronze_per_energy": S.BRONZE_PER_ENERGY, "gacha_cost": S.GACHA_COST, "secret_cost": S.SECRET_COST,
        "epilogue_cost": S.EPILOGUE_COST, "streak_caps": S.STREAK_CAPS,
        "new_player_crystals": S.NEW_PLAYER_CRYSTALS, "new_player_gold_tickets": S.NEW_PLAYER_GOLD_TICKETS,
        "panels_per_picture": S.PANELS_PER_PICTURE, "default_costume": S.DEFAULT_COSTUME,
        "card_suffix": S.CARD_SUFFIX, "default_bg": S.DEFAULT_BG, "default_face": S.DEFAULT_FACE,
        "courses": S.COURSES, "news": S.NEWS,
        "heroines": heroines, "overlays": overlays, "stories": stories, "photos": photos,
        "backgrounds": [{"name": k, "url": f"/static/{v}/{u}" if u else None} for k, u in S.BACKGROUNDS.items()],
        "events": [{**e, "banner_url": f"/static/{v}/{e['banner']}" if e.get("banner") else None} for e in S.EVENTS],
        "main_story_list": [{k: s.get(k) for k in ("scenario_id", "title", "chapter", "partner_name", "story_type")}
                            for s in S.STORIES],
    })
    shutil.copy(os.path.join(R, "..", "server", "templates", "index.html"), os.path.join(OUT, "index.html"))
    print(len(sids), "scenarios,", len(stories), "stories,", len(overlays), "face overlays")


if __name__ == "__main__":
    main()
