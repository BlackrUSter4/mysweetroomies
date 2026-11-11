"""Smoke tests that talk to the server exactly like the Android client would.

Run:  python -m pytest test_server.py
"""

import json
import tempfile
from pathlib import Path

import app as server
import yoga_crypto

KEY = bytes.fromhex(yoga_crypto.ORIGINAL_KEY_HEX)


def client_headers(uid="11111111-2222-3333-4444-555555555555"):
    auth = json.dumps({"device_id": "abc", "timestamp": "2015-01-01 12:00:00", "uuid": uid})
    return {
        "X-Yoga-Authorization": yoga_crypto.encrypt(KEY, auth),
        "X-Package-Name": "jp.mfapps.novel.roomies",
        "X-Version-Code": "146",
        "X-Version-Name": "1.4.6",
        "X-Yoga-Request-From": "native",
    }


def make_client():
    server.DB_PATH = Path(tempfile.mkdtemp()) / "test.sqlite3"
    server.init_db()
    server.app.config["TESTING"] = True
    c = server.app.test_client()
    c.get("/", headers=client_headers())  # creates the player
    return c


def state(c):
    return c.get("/api/me", headers=client_headers()).get_json()


def test_crypto_roundtrip():
    assert yoga_crypto.decrypt(KEY, yoga_crypto.encrypt(KEY, "héllo")) == "héllo"


def test_unauthorized_matches_archived_server():
    c = server.app.test_client()
    r = c.get("/")
    assert r.status_code == 401 and r.data == b"Unauthorized"


def test_top_page_and_invite_id():
    c = make_client()
    r = c.get("/?__t=1", headers=client_headers())
    assert r.status_code == 200 and b"My Sweet Roomies" in r.data
    assert len(c.get("/my/", headers=client_headers()).get_json()["content"]["invite_id"]) == 8


def test_heroine_ids_match_official_site():
    names = {h["id"]: h["short"] for h in server.HEROINES}
    assert names == {"01": "Ren", "02": "Aki", "03": "Mitsuka", "04": "Aoi", "05": "Sayaka", "06": "Misaki"}
    sid = server.story_id("03", 2, 1)
    assert len(sid) == 12 and sid[2:4] == "03" and sid[8:10] == "02"  # MainStoryJson offsets


def test_route_lengths_and_titles_from_playthroughs():
    mains = lambda h: {s["chapter"] for s in server.STORIES if s["heroine_id"] == h and s["kind"] == "main"}
    # Aoi skips card numbers 27/32 and prints 34 twice; Aki skips 36.
    assert {h: len(mains(h)) for h in ("01", "02", "03", "04", "05", "06")} ==         {"01": 53, "02": 57, "03": 52, "04": 55, "05": 52, "06": 54}
    assert server.parts_for("02", 11) == 7          # matches a player's own notes ("home party 1-7")
    first = server.STORY_BY_ID[server.story_id("02", 1, 1)]
    assert first["title"] == "Chapter 1 - My Famous Roommate"


def test_real_prologue_transcript():
    c = make_client()
    r = c.get("/scenarios/530000000001/transcript", headers=client_headers())
    # NetworkClient.isJsonResponse() rejects a body that doesn't end exactly in "}".
    assert r.get_data().endswith(b"}")
    t = r.get_json()["content"]
    # Layout verified against libotome.so (ExScenarioData::parseJson): text lives in "scene".
    scenes = [x["scene"] for x in t["scenario_data"]]
    line = next(s for s in scenes if s["body"] == '"A truck smashed into your room!?"')
    assert line["speaker"] == "???" and line["show_tail"] == "speak"
    assert scenes[0]["speaker"] == "__NULL__" and scenes[0]["show_tail"] == "none"
    assert t["scenario_data"][-1]["visual"]["end_mark"] == {"type": "continue"}
    # absent visual effects are omitted, never null (the parser reads them as objects)
    assert all(v is not None for x in t["scenario_data"] for v in x.get("visual", {}).values())


def test_story_pipeline():
    c = make_client()
    h = client_headers()
    assert "01" in c.get("/resource/character_offset", headers=h).get_json()["content"]
    sid = server.story_id("01", 1, 1)
    assert c.get(f"/scenarios/{sid}/transcript", headers=h).get_json()["content"]["scenario_id"] == sid
    rl = c.get(f"/scenarios/{sid}/resource_list", headers=h).get_json()
    assert rl["static_version"] != 0 and rl["static_hostname"]
    assert rl["content"]["next_scenario_id_list"] == [server.story_id("01", 1, 2)]


def test_energy_spend_lock_and_recover():
    c = make_client()
    h = client_headers()
    read = lambda sid: c.post(f"/api/story/{sid}/read", headers=h)
    assert read(server.story_id("04", 1, 1)).status_code == 403          # prologue first
    assert read("530000000001").status_code == 200
    assert read("530000000002").status_code == 200
    assert read(server.story_id("04", 1, 2)).status_code == 403          # parts in order
    order = [s["scenario_id"] for s in server.STORIES if s["heroine_id"] == "04"]
    n = server.TICKET_MAX - 2                                              # 5 energy - 2 prologue parts
    for sid in order[:n]:
        assert read(sid).status_code == 200
    assert state(c)["tickets"]["tickets"] == 0
    assert read(order[n]).status_code == 409
    assert read("530000000001").status_code == 200                       # replay is free
    assert c.post("/api/energy/recover", headers=h, json={"method": "crystal"}).status_code == 200
    s = state(c)
    assert s["tickets"]["tickets"] == 1 and s["crystals"] == server.NEW_PLAYER_CRYSTALS - 1
    assert c.post("/api/energy/recover", headers=h, json={"method": "gold_ticket"}).status_code == 200
    assert state(c)["tickets"]["tickets"] == server.TICKET_MAX
    assert c.post("/api/energy/recover", headers=h, json={"method": "bronze_ticket"}).status_code == 402


def test_exchange_items():
    c = make_client()
    h = client_headers()
    assert c.post("/api/shop/exchange/gold_ticket", headers=h).status_code == 200
    s = state(c)
    assert s["crystals"] == server.NEW_PLAYER_CRYSTALS - 5 and s["items"]["gold_ticket"] == 2


def test_ticket_regen_is_lazy():
    user = {"tickets": 2, "tickets_at": 1000}
    t = server.ticket_state(user, 1000 + 2 * server.TICKET_REGEN_SECONDS + 5)
    assert t["tickets"] == 4 and t["next_in"] == server.TICKET_REGEN_SECONDS - 5


def test_payment_post_grants_crystals_and_gacha():
    c = make_client()
    h = client_headers()
    purchase = json.dumps({"productId": "jp.mfapps.novel.roomies.item.credit.15", "orderId": "GPA.1"})
    r = c.post("/payment/android/purchase", headers=h,
               data={"purchase_data": yoga_crypto.encrypt(KEY, purchase)})
    assert r.status_code == 200
    assert state(c)["crystals"] == server.NEW_PLAYER_CRYSTALS + 15
    r = c.post("/api/gacha/02", headers=h)
    assert r.status_code == 200 and r.get_json()["costume"] in server.COSTUMES["02"]
    r = c.post("/api/gacha/01", headers=h)                                     # Ren's six (Mega Assets pack)
    assert r.status_code == 200 and r.get_json()["costume"] in server.COSTUMES["01"]
    assert c.get(r.get_json()["url"]).status_code == 200
    won = r.get_json()["costume"]
    r = c.post("/api/costume/01", headers=h, json={"costume": won})
    ren = next(x for x in r.get_json()["heroines"] if x["id"] == "01")
    assert ren["costume"] == won and ren["stand_url"].endswith(f"/costume/01_{server.COSTUMES['01'][won]}.png")
    for hid, pool in server.ALL_COSTUMES.items():                              # every outfit has its picture
        for f in pool.values():
            assert (server.STATIC_DIR / f"img/heroine/costume/{hid}_{f}.png").is_file(), (hid, f)


def test_card_versions_unlock_with_their_outfit():
    c = make_client()
    h = client_headers()
    state(c)
    card = "Wedding Dress" + server.CARD_SUFFIX
    assert c.post("/api/costume/04", headers=h, json={"costume": card}).status_code == 404    # not owned yet
    conn = server.sqlite3.connect(server.DB_PATH)                              # as if the gacha gave it
    conn.execute("INSERT OR IGNORE INTO costumes SELECT uuid, '04', 'Wedding Dress', 0 FROM users")
    conn.commit()
    conn.close()
    aoi = next(x for x in state(c)["heroines"] if x["id"] == "04")
    assert card in aoi["costumes"] and aoi["costume_pool"].index(card) == aoi["costume_pool"].index("Wedding Dress") + 1
    r = c.post("/api/costume/04", headers=h, json={"costume": card})
    aoi = next(x for x in r.get_json()["heroines"] if x["id"] == "04")
    assert aoi["costume"] == card and aoi["stand_url"].endswith("/costume/04_wedding.png") and not aoi["face_overlay"]
    for hid, pool in server.CARD_ART.items():                                  # cards are never gacha prizes
        assert not any(n.endswith(server.CARD_SUFFIX) for n in server.COSTUMES[hid])
        assert all(n in server.COSTUMES[hid] for n in pool)


def test_touch_scene_plays_as_scenario():
    c = make_client()
    h = client_headers()
    aki = next(x for x in state(c)["heroines"] if x["id"] == "02")
    sid = aki["touch_id"]
    data = c.get(f"/scenarios/{sid}/transcript", headers=h).get_json()["content"]["scenario_data"]
    assert data[0]["scene"]["body"].startswith("\"I'm finally at home!")
    assert data[-1]["visual"]["end_mark"]["type"] == "none"
    assert "/img/resource/bg/Touch_02_001.png" in c.get(f"/scenarios/{sid}/resource_list", headers=h).get_json()["content"]["resources"]


def test_daily_login_bonus_once_per_day():
    c = make_client()
    h = client_headers()
    assert c.post("/api/login_bonus", headers=h).get_json()["granted"] is True
    assert c.post("/api/login_bonus", headers=h).get_json()["granted"] is False


def test_bad_story_id_rejected():
    c = make_client()
    assert c.get("/scenarios/..%2f..%2fetc/transcript").status_code in (400, 404)


def test_scene_pictures_and_secret_story():
    c = make_client()
    h = client_headers()
    # A part with recovered pictures lists them for download, and every bg it uses is on disk.
    sid = server.story_id("01", 20, 1)
    res = c.get(f"/scenarios/{sid}/resource_list", headers=h).get_json()["content"]["resources"]
    tr = c.get(f"/scenarios/{sid}/transcript", headers=h).get_json()["content"]["scenario_data"]
    used = {f"/img/resource/bg/{x['scene']['bg']}.png" for x in tr if x["scene"].get("bg")}
    if used:
        assert used <= set(res)
        assert tr[0]["scene"]["body"].startswith("Chapter 20")
    # Aki's Secret Story 1 opens after Chapter 4 part 3 and ends with "To Be Continued".
    sec = [s for s in server.STORIES if s["kind"] == "secret" and s["heroine_id"] == "02" and s["part"] == 1]
    assert sec and sec[0]["chapter"] == 4 and server.previous_story(sec[0]) == server.story_id("02", 4, 3)
    assert len([s for s in server.STORIES if s["kind"] == "secret"]) == 56
    rows = {s["scenario_id"]: s for s in c.get("/api/stories/02", headers=h).get_json()["stories"]}
    assert rows[sec[0]["scenario_id"]]["locked"]
    tr = c.get(f"/scenarios/{sec[0]['scenario_id']}/transcript", headers=h).get_json()["content"]["scenario_data"]
    assert tr[-1]["visual"]["end_mark"]["type"] == "continue"


def test_memory_pass_item():
    c = make_client()
    h = client_headers("22222222-2222-3333-4444-555555555555")
    s0 = c.get("/api/me", headers=h).get_json()
    st = s0.get("state", s0)
    assert st["memory_pass"] is False
    r = c.post("/api/shop/exchange/memory_pass_30", headers=h)
    assert r.status_code == 200
    st = r.get_json()["state"]
    assert st["memory_pass"] is True and st["crystals"] == server.NEW_PLAYER_CRYSTALS - 9
    assert c.post("/api/shop/exchange/memory_pass_30", headers=h).status_code == 402   # 1 crystal left


def test_photo_panel_pictures():
    c = make_client()
    h = client_headers()
    s = state(c)
    assert s["panels"]["total"] == 16 and s["panels"]["current"]["caption"] == "Aoi : Wake-Up Call"
    assert [p["kind"] for p in s["photos"]] == ["event"] * 3     # recovered event pictures are always shown
    db = server.sqlite3.connect(server.DB_PATH)
    db.execute("UPDATE users SET panel_tickets = 16"); db.commit(); db.close()
    for _ in range(16):
        assert c.post("/api/panels/use_ticket", headers=h).status_code == 200
    s = state(c)
    assert s["panels"]["completed"] == 1 and s["photos"][0]["sub"] == "2018/06/09-2018/06/23"
    assert len(s["photos"]) == 4
    assert c.get(s["photos"][0]["url"]).status_code == 200
    for p in server.PHOTOS:                      # every picture (Aoi's + the Mega Assets ones) has its file
        assert (server.STATIC_DIR / f"img/photo/{p['name']}.png").is_file(), p["name"]
    untitled = server.photo_entry(next(p for p in server.PHOTOS if p["title"] is None))
    assert ":" not in untitled["caption"] and untitled["sub"] == "Title not recorded"


def test_login_streak_raises_energy_cap():
    user = {"course": None, "login_streak": 0}
    caps = {d: server.ticket_max({**user, "login_streak": d}) for d in (1, 2, 4, 5, 8, 20)}
    assert caps == {1: 5, 2: 6, 4: 6, 5: 7, 8: 8, 20: 8}


def test_epilogue_costs_crystals():
    c = make_client()
    h = client_headers()
    ep = next(s for s in server.STORIES if s["kind"] == "epilogue")
    db = server.sqlite3.connect(server.DB_PATH)
    prev = server.previous_story(ep)
    db.execute("INSERT INTO story_read SELECT uuid, ?, 0 FROM users", (prev,))
    db.execute("UPDATE users SET crystals = 3"); db.commit()
    assert c.post(f"/api/story/{ep['scenario_id']}/read", headers=h).status_code == 402
    db.execute("UPDATE users SET crystals = 12"); db.commit(); db.close()
    assert c.post(f"/api/story/{ep['scenario_id']}/read", headers=h).status_code == 200
    assert state(c)["crystals"] == 2


def test_secret_story_costs_crystals():
    # English popup "Get the Secret Story!!": 5 crystals ("You need N crystal(s) more!").
    c = make_client()
    h = client_headers()
    sec = next(s for s in server.STORIES if s["kind"] == "secret" and s["heroine_id"] == "04" and s["part"] == 1)
    db = server.sqlite3.connect(server.DB_PATH)
    db.execute("INSERT INTO story_read SELECT uuid, ?, 0 FROM users", (server.previous_story(sec),))
    db.execute("UPDATE users SET crystals = 3"); db.commit()
    r = c.post(f"/api/story/{sec['scenario_id']}/read", headers=h)
    assert r.status_code == 402 and "2 crystal(s) more" in r.get_json()["message"]
    db.execute("UPDATE users SET crystals = 7"); db.commit(); db.close()
    assert c.post(f"/api/story/{sec['scenario_id']}/read", headers=h).status_code == 200
    assert state(c)["crystals"] == 2
    tr = c.get(f"/scenarios/{sec['scenario_id']}/transcript", headers=h).get_json()["content"]["scenario_data"]
    assert any("Seeing William off" in x["scene"].get("body", "") for x in tr[:3])   # Aoi Secret Story 1, recovered 2026-10-12


def test_epilogue_1_opens_after_first_half():
    # Ren's first half ends at Chapter 31 (JP 31 -> 32 「隣に咲く花」 = "The Flower Blooming Nearby")
    ep1 = next(s for s in server.STORIES if s["kind"] == "epilogue" and s["heroine_id"] == "01" and s["chapter"] == 1)
    ep2 = next(s for s in server.STORIES if s["kind"] == "epilogue" and s["heroine_id"] == "01" and s["chapter"] == 2)
    prev1 = server.STORY_BY_ID[server.previous_story(ep1)]
    assert server.chapters_for("01")[prev1["chapter"] - 1]["label"] == "31"
    last = [s for s in server.STORIES if s["heroine_id"] == "01" and s["kind"] == "main"][-1]
    assert server.previous_story(ep2) == last["scenario_id"]
    # Chapter 32 doesn't wait for the epilogue
    ch32 = next(s for s in server.STORIES if s["heroine_id"] == "01" and s["kind"] == "main" and s["chapter"] == prev1["chapter"] + 1)
    assert server.STORY_BY_ID[server.previous_story(ch32)]["kind"] == "main"


def test_login_bonus_item_every_second_day():
    c = make_client()
    h = client_headers()
    r = c.post("/api/login_bonus", headers=h).get_json()
    assert r["granted"] and r["item"] is None
    db = server.sqlite3.connect(server.DB_PATH)
    db.execute("UPDATE users SET last_login_day = '2000-01-01'"); db.commit(); db.close()
    r = c.post("/api/login_bonus", headers=h).get_json()
    assert r["item"] == "silver_ticket" and r["state"]["items"]["silver_ticket"] == 1


def test_touch_chat_lines_for_every_heroine():
    hs = {h["id"]: h for h in state(make_client())["heroines"]}
    assert all(hs[i]["chat"] for i in ("01", "02", "03", "04", "05", "06"))
    # All 82 voiced lines of the Japanese APK (Ren 13 ... Misaki 7), each with a playable clip.
    assert sum(len(hs[i]["chat"]) for i in hs) == 82 and len(hs["03"]["chat"]) == 19 and len(hs["06"]["chat"]) == 7
    c = make_client()
    line = next(x for x in hs["02"]["chat"] if "ice cream" in x["text"])      # official English (Aki)
    assert c.get(f"/static/1/sound/voice/mypage/{line['voice']}.ogg").status_code == 200


def test_event_stories_listed_with_banners():
    # Three うちカノ banners from the Mobage fan circle. Every 『ファミーユ』 event has one of its promo cards (Mega Assets
    # pack, research/event_banners.py) except the Valentine's one and the hot-spring and Christmas events, which are
    # known only from TW recordings.
    c = make_client()
    ev = c.get("/api/events", headers=client_headers()).get_json()["events"]
    assert len(ev) == 23 and {e["game"] for e in ev} == {"famille", "uchikano"}
    assert [e["id"] for e in ev if e["game"] == "famille" and not e["banner_url"]] == [
        "famille_hotspring_trip", "famille_dangerous_christmas", "famille_their_valentines"]
    banners = [e["banner_url"] for e in ev if e["banner_url"]]
    assert len(banners) == 18 and all(c.get(u).status_code == 200 for u in banners)


def test_dress_up_background():
    # Dress Up > Background (Mega Assets store ad): house, garden, beach recovered; bridge and cherry locked.
    c = make_client()
    h = client_headers()
    s = state(c)
    assert all(x["bg"] == server.DEFAULT_BG for x in s["heroines"])
    assert [b["name"] for b in s["backgrounds"] if not b["url"]] == ["Sunset Bridge", "Cherry Blossoms", "Sayaka's Room",
                                                                     "Balcony (Morning)"]
    assert all(c.get(b["url"]).status_code == 200 for b in s["backgrounds"] if b["url"])          # 11 with art
    assert c.post("/api/dress_bg/02", headers=h, json={"bg": "Cherry Blossoms"}).status_code == 404
    r = c.post("/api/dress_bg/02", headers=h, json={"bg": "Beach"})
    aki = next(x for x in r.get_json()["heroines"] if x["id"] == "02")
    assert aki["bg"] == "Beach" and c.get(aki["bg_url"]).status_code == 200
    assert next(x for x in r.get_json()["heroines"] if x["id"] == "01")["bg"] == server.DEFAULT_BG   # per heroine


def test_dress_up_face():
    # Dress Up > Face (research/face_items.py): the 8 named expressions per heroine, laid over Taiwan-wardrobe
    # outfits only (they share the story sprites' frontal head); "Default" is the outfit's own face.
    c = make_client()
    h = client_headers()
    ren = next(x for x in state(c)["heroines"] if x["id"] == "01")
    assert ren["face"] == "Default" and ren["face_overlay"] is None and len(ren["face_pool"]) == 8
    assert all(c.get(f["url"]).status_code == 200 for f in ren["face_pool"] if f["url"])
    assert c.post("/api/dress_face/01", headers=h, json={"face": "Grinning"}).status_code == 404
    r = c.post("/api/dress_face/01", headers=h, json={"face": "Smile"})
    ren = next(x for x in r.get_json()["heroines"] if x["id"] == "01")
    assert ren["face"] == "Smile" and ren["face_overlay"] is None        # the default outfit has no face layer
    with server.app.test_request_context():
        o = server.face_overlay("01", next(k for k, v in server.COSTUMES["01"].items() if v.startswith("tw")), "Smile")
    assert o and o["w"] > 100 and 0 <= o["x"] < o["iw"] and 0 <= o["y"] < o["ih"]
    for hid, d in server.FACES.items():                                  # every face has its picture
        for f in d["faces"].values():
            assert (server.STATIC_DIR / f"img/heroine/face/{f}").is_file(), f


def test_recovered_secret_stories_and_event_preview():
    # Taiwan-edition recordings (research/tw_secret_stories.py): Aki 2-3, Sayaka 1-10, Misaki 1-3, the Edo event preview.
    c = make_client()
    for sid in ("530203010602", "530503010301", "530503014910", "530603010901", server.EVENT_SCENARIOS and "530008010101"):
        assert (server.SCENARIO_DIR / sid / "transcript.json").is_file(), sid
    assert server.end_mark_type("530008010101") == "end"
    ev = c.get("/api/events", headers=client_headers()).get_json()["events"]
    assert [e["id"] for e in ev if e.get("scenario_id")] == ["famille_edo"]


def test_eighth_pass_secret_stories():
    # Taiwan-edition recordings (research/tw_secret_stories2.py): Ren 1-5 (CK channel) and Aoi 2 (breeze llc).
    sec = [s for s in server.STORIES if s["kind"] == "secret" and
           ((s["heroine_id"] == "01" and s["part"] <= 5) or (s["heroine_id"] == "04" and s["part"] == 2))]
    assert len(sec) == 6
    for s in sec:
        assert (server.SCENARIO_DIR / s["scenario_id"] / "transcript.json").is_file(), s["scenario_id"]


def test_hotspring_event_routes():
    # breeze llc recordings (research/tw_hotspring.py): Sayaka's and Aoi's 20-part routes; the last part ends on Fin.
    c = make_client()
    ev = next(e for e in c.get("/api/events", headers=client_headers()).get_json()["events"]
              if e["id"] == "famille_hotspring_trip")
    assert len(ev["parts"]) == 40 and {p["heroine_id"] for p in ev["parts"]} == {"04", "05"}
    for p in ev["parts"]:
        assert (server.SCENARIO_DIR / p["scenario_id"] / "transcript.json").is_file(), p["scenario_id"]
    assert server.end_mark_type("530009050101") == "continue" and server.end_mark_type("530009052001") == "end"


def test_christmas_event_ren_route():
    # 大熊猫中国 recordings (research/tw_xmas.py): Ren's 20 episodes of 兩個人的危險聖誕節.
    c = make_client()
    ev = next(e for e in c.get("/api/events", headers=client_headers()).get_json()["events"]
              if e["id"] == "famille_dangerous_christmas")
    assert len(ev["parts"]) == 20 and {p["heroine_id"] for p in ev["parts"]} == {"01"}
    for p in ev["parts"]:
        assert (server.SCENARIO_DIR / p["scenario_id"] / "transcript.json").is_file(), p["scenario_id"]
    assert server.end_mark_type("530010010101") == "continue" and server.end_mark_type("530010012001") == "end"


def test_ama_stream_event_episodes():
    # A機's 2016 streams (research/tw_ama_events.py): Seaside Cinderella episode 1s and Summer Festival episodes.
    c = make_client()
    ev = {e["id"]: e for e in c.get("/api/events", headers=client_headers()).get_json()["events"]}
    sea = ev["famille_seaside_cinderella"]["parts"]
    assert len(sea) >= 56 and len(ev["famille_summer_together"]["parts"]) >= 8
    assert {"Ren Secret Story", "Sayaka Episode 20", "Early-Clear Bonus Story"} <= {p["title"] for p in sea}
    for e in ("famille_seaside_cinderella", "famille_summer_together"):
        for p in ev[e]["parts"]:
            assert (server.SCENARIO_DIR / p["scenario_id"] / "transcript.json").is_file(), p["scenario_id"]


def test_edo_event_sayaka_episode():
    # Edo event gacha recording (research/tw_edo_gacha.py): Sayaka's episode 1 is a second part of the event,
    # ending "To Be Continued" (the preview ends with the Fin card).
    c = make_client()
    edo = next(e for e in c.get("/api/events", headers=client_headers()).get_json()["events"] if e["id"] == "famille_edo")
    assert [p["scenario_id"] for p in edo["parts"]][:2] == ["530008010101", "530008050101"]
    assert "530008020101" in [p["scenario_id"] for p in edo["parts"]]       # Aki episode 1 (research/tw_edo_aki.py)
    t = json.load(open(server.SCENARIO_DIR / "530008050101" / "transcript.json", encoding="utf8"))
    assert len(t["scenario_data"]) == 16 and t["scenario_data"][-1]["speaker"] == "Sayaka"
    assert server.end_mark_type("530008050101") == "continue" and server.end_mark_type("530008010101") == "end"


def test_player_name_replaces_william_in_stories():
    c = make_client()
    h = client_headers()
    sid = "530401010101"                                                       # Aoi Chapter 1 part 1
    body = lambda: " ".join(x["scene"]["body"] + " " + x["scene"]["speaker"]   # noqa: E731
                            for x in c.get(f"/scenarios/{sid}/transcript", headers=h).get_json()["content"]["scenario_data"])
    assert "William" in body()
    assert c.post("/api/name", headers=h, json={"name": "Mihile"}).status_code == 200
    text = body()
    assert "William" not in text and "Mihile" in text


def test_soundtrack_catalog():
    c = make_client()
    r = c.get("/api/soundtrack", headers=client_headers()).get_json()
    tracks = r["tracks"]
    assert len(tracks) == 21                                                   # the full official soundtrack
    assert tracks[0]["category"] == "main"                                     # main theme listed first
    # every track resolves to an .ogg the static route serves, and one per heroine is a "Theme 1"
    for t in tracks:
        assert t["url"] == f"/static/{server.STATIC_VERSION}/{t['file']}"
        assert (server.STATIC_DIR / t["file"]).is_file()
    assert set(server.HEROINE_BGM) == {"01", "02", "03", "04", "05", "06"}


def test_story_plays_heroine_theme():
    c = make_client()
    h = client_headers()
    first_bgm = lambda sid: next(                                              # noqa: E731
        (x["audio"]["bgm"] for x in c.get(f"/scenarios/{sid}/transcript", headers=h)
         .get_json()["content"]["scenario_data"] if x.get("audio", {}).get("bgm")), None)
    assert first_bgm(server.story_id("04", 1, 1)) == server.HEROINE_BGM["04"]  # Aoi -> her theme
    assert first_bgm("530000000001") == server.MAIN_BGM                        # shared prologue -> main theme
