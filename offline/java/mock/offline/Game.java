package mock.offline;

import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Base64;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.crypto.Cipher;
import javax.crypto.Mac;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

import mock.offline.HttpServer.Req;
import mock.offline.HttpServer.Resp;

/**
 * The game rules of server/app.py, ported for the offline APK. Everything that only depends on fixed data
 * (story list and unlock order, transcripts, picture lists, outfit/face/background tables) was exported by
 * offline/export_data.py into assets/server; this class keeps the per-player state in SQLite and answers the
 * same routes with the same JSON. When app.py changes, re-run export_data.py, and port rule changes here.
 */
final class Game {
    private static final String TAG = HttpServer.TAG;
    private final SQLiteDatabase db;
    private final JSONObject S;
    private final byte[] aesKey;
    private final byte[] sessionKey;
    private final String indexHtml;
    private final Random random = new Random();

    private final List<JSONObject> stories = new ArrayList<>();
    private final Map<String, JSONObject> storyById = new HashMap<>();
    private final Map<String, Integer> storyIndex = new HashMap<>();
    private final List<JSONObject> heroines = new ArrayList<>();          // static.json "heroines" entries
    private final Map<String, JSONObject> heroineById = new HashMap<>();  // id -> entry
    private final LinkedHashMap<String, String> backgrounds = new LinkedHashMap<>();   // name -> url or null
    private final List<JSONObject> panelPhotos = new ArrayList<>(), eventPhotos = new ArrayList<>(),
            epiloguePhotos = new ArrayList<>();

    Game(Context ctx) throws IOException, JSONException {
        S = new JSONObject(new String(HttpServer.readAsset("server/static.json"), HttpServer.UTF8));
        indexHtml = new String(HttpServer.readAsset("server/index.html"), HttpServer.UTF8);
        JSONArray st = S.getJSONArray("stories");
        for (int i = 0; i < st.length(); i++) {
            JSONObject s = st.getJSONObject(i);
            stories.add(s);
            storyById.put(s.getString("scenario_id"), s);
            storyIndex.put(s.getString("scenario_id"), i);
        }
        JSONArray hs = S.getJSONArray("heroines");
        for (int i = 0; i < hs.length(); i++) {
            JSONObject h = hs.getJSONObject(i);
            heroines.add(h);
            heroineById.put(h.getJSONObject("base").getString("id"), h);
        }
        JSONArray bgs = S.getJSONArray("backgrounds");
        for (int i = 0; i < bgs.length(); i++) {
            JSONObject b = bgs.getJSONObject(i);
            backgrounds.put(b.getString("name"), b.isNull("url") ? null : b.getString("url"));
        }
        JSONArray ph = S.getJSONArray("photos");
        for (int i = 0; i < ph.length(); i++) {
            JSONObject p = ph.getJSONObject(i);
            String k = p.getString("kind");
            if (k.equals("panel")) panelPhotos.add(p);
            else if (k.equals("event")) eventPhotos.add(p);
            else if (k.equals("epilogue")) epiloguePhotos.add(p);
        }
        aesKey = signatureKey(ctx);
        db = ctx.openOrCreateDatabase("mock.sqlite3", Context.MODE_PRIVATE, null);
        initDb();
        sessionKey = sessionSecret();
        Log.i(TAG, "game data: " + stories.size() + " stories, AES key " + hex(aesKey));
    }

    // ------------------------------------------------------------------ setup

    /** Security.getApplicationSignature(): MD5 of the signing certificate's hex string (yoga_crypto.key_from_apk). */
    @SuppressWarnings("deprecation")
    private static byte[] signatureKey(Context ctx) {
        try {
            PackageInfo pi = ctx.getPackageManager().getPackageInfo(ctx.getPackageName(), PackageManager.GET_SIGNATURES);
            return MessageDigest.getInstance("MD5").digest(pi.signatures[0].toCharsString().getBytes("US-ASCII"));
        } catch (Exception e) {
            Log.e(TAG, "signature", e);
            return hexBytes("e9ee6770731a5c5b7dff9b7f827cadb8");
        }
    }

    private void initDb() {
        db.execSQL("CREATE TABLE IF NOT EXISTS users (uuid TEXT PRIMARY KEY, device_id TEXT, invite_id TEXT NOT NULL,"
                + " name TEXT NOT NULL DEFAULT 'William', heroine_id TEXT NOT NULL DEFAULT '04',"
                + " crystals INTEGER NOT NULL DEFAULT 0, gold_tickets INTEGER NOT NULL DEFAULT 0,"
                + " silver_tickets INTEGER NOT NULL DEFAULT 0, bronze_tickets INTEGER NOT NULL DEFAULT 0,"
                + " panel_tickets INTEGER NOT NULL DEFAULT 0, course TEXT, tickets INTEGER NOT NULL,"
                + " tickets_at INTEGER NOT NULL, panels INTEGER NOT NULL DEFAULT 0, pictures_done INTEGER NOT NULL DEFAULT 0,"
                + " last_login_day TEXT, created_at INTEGER NOT NULL, last_seen INTEGER NOT NULL, pass_until INTEGER,"
                + " login_days INTEGER NOT NULL DEFAULT 0, login_streak INTEGER NOT NULL DEFAULT 0)");
        db.execSQL("CREATE TABLE IF NOT EXISTS story_read (uuid TEXT NOT NULL, scenario_id TEXT NOT NULL,"
                + " read_at INTEGER NOT NULL, PRIMARY KEY (uuid, scenario_id))");
        db.execSQL("CREATE TABLE IF NOT EXISTS costumes (uuid TEXT NOT NULL, heroine_id TEXT NOT NULL, costume TEXT NOT NULL,"
                + " equipped INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (uuid, heroine_id, costume))");
        db.execSQL("CREATE TABLE IF NOT EXISTS dress_bg (uuid TEXT NOT NULL, heroine_id TEXT NOT NULL, bg TEXT NOT NULL,"
                + " PRIMARY KEY (uuid, heroine_id))");
        db.execSQL("CREATE TABLE IF NOT EXISTS dress_face (uuid TEXT NOT NULL, heroine_id TEXT NOT NULL, face TEXT NOT NULL,"
                + " PRIMARY KEY (uuid, heroine_id))");
        db.execSQL("CREATE TABLE IF NOT EXISTS purchases (id INTEGER PRIMARY KEY AUTOINCREMENT, uuid TEXT, product_id TEXT,"
                + " raw TEXT, created_at INTEGER NOT NULL)");
        db.execSQL("CREATE TABLE IF NOT EXISTS meta (k TEXT PRIMARY KEY, v TEXT)");
    }

    private byte[] sessionSecret() {
        String v = scalar("SELECT v FROM meta WHERE k = 'session_key'");
        if (v == null) {
            byte[] b = new byte[32];
            new SecureRandom().nextBytes(b);
            v = hex(b);
            db.execSQL("INSERT OR REPLACE INTO meta VALUES ('session_key', ?)", new Object[]{v});
        }
        return hexBytes(v);
    }

    // ------------------------------------------------------------------ helpers

    private static long now() { return System.currentTimeMillis() / 1000L; }

    private String scalar(String sql, String... args) {
        Cursor c = db.rawQuery(sql, args);
        try { return c.moveToFirst() && !c.isNull(0) ? c.getString(0) : null; } finally { c.close(); }
    }

    private static Map<String, Object> row(Cursor c) {
        Map<String, Object> m = new HashMap<>();
        for (int i = 0; i < c.getColumnCount(); i++) {
            switch (c.getType(i)) {
                case Cursor.FIELD_TYPE_NULL: m.put(c.getColumnName(i), null); break;
                case Cursor.FIELD_TYPE_INTEGER: m.put(c.getColumnName(i), c.getLong(i)); break;
                default: m.put(c.getColumnName(i), c.getString(i));
            }
        }
        return m;
    }

    private List<Map<String, Object>> rows(String sql, String... args) {
        List<Map<String, Object>> out = new ArrayList<>();
        Cursor c = db.rawQuery(sql, args);
        try { while (c.moveToNext()) out.add(row(c)); } finally { c.close(); }
        return out;
    }

    private static long L(Map<String, Object> u, String k) { Object v = u.get(k); return v == null ? 0 : ((Number) v).longValue(); }
    private static String Str(Map<String, Object> u, String k) { Object v = u.get(k); return v == null ? null : v.toString(); }
    private static Object nul(Object o) { return o == null ? JSONObject.NULL : o; }

    private static JSONObject jo(Object... kv) {
        JSONObject o = new JSONObject();
        try { for (int i = 0; i < kv.length; i += 2) o.put((String) kv[i], nul(kv[i + 1])); } catch (JSONException e) { throw new RuntimeException(e); }
        return o;
    }

    private static Resp json(int status, Object o) { return Resp.json(status, o.toString()); }
    private static Resp json(Object o) { return json(200, o); }

    private Resp proxyJson(Object content, Object... extra) {
        JSONObject o = jo("api_version", S.optInt("api_version", 5), "timestamp", now(), "version", 1, "cache_lifetime", 0,
                "is_maintenance", 0, "error", null, "location", null, "content", content);
        try { for (int i = 0; i < extra.length; i += 2) o.put((String) extra[i], nul(extra[i + 1])); } catch (JSONException e) { throw new RuntimeException(e); }
        return json(o);
    }

    private JSONObject readJsonAsset(String path) {
        try {
            if (!HttpServer.assetExists(path)) return null;
            return new JSONObject(new String(HttpServer.readAsset(path), HttpServer.UTF8));
        } catch (Exception e) {
            Log.w(TAG, "asset " + path + ": " + e);
            return null;
        }
    }

    static String hex(byte[] b) {
        StringBuilder s = new StringBuilder();
        for (byte x : b) s.append(String.format(Locale.US, "%02x", x & 0xff));
        return s.toString();
    }

    static byte[] hexBytes(String h) {
        byte[] b = new byte[h.length() / 2];
        for (int i = 0; i < b.length; i++) b[i] = (byte) Integer.parseInt(h.substring(2 * i, 2 * i + 2), 16);
        return b;
    }

    private String decrypt(String token) throws Exception {
        Cipher c = Cipher.getInstance("AES/CBC/PKCS5Padding");
        c.init(Cipher.DECRYPT_MODE, new SecretKeySpec(aesKey, "AES"), new IvParameterSpec("0123456789abcdef".getBytes("US-ASCII")));
        byte[] raw = Base64.decode(token.trim(), Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING);
        return new String(c.doFinal(raw), HttpServer.UTF8);
    }

    private String sign(String uuid) {
        try {
            Mac m = Mac.getInstance("HmacSHA256");
            m.init(new SecretKeySpec(sessionKey, "HmacSHA256"));
            String id = Base64.encodeToString(uuid.getBytes(HttpServer.UTF8), Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING);
            String mac = Base64.encodeToString(m.doFinal(id.getBytes(HttpServer.UTF8)), Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING);
            return id + "." + mac.substring(0, 22);
        } catch (Exception e) { throw new RuntimeException(e); }
    }

    private String unsign(String token) {
        int d = token.lastIndexOf('.');
        if (d < 0) return null;
        String uuid = new String(Base64.decode(token.substring(0, d), Base64.URL_SAFE | Base64.NO_WRAP | Base64.NO_PADDING), HttpServer.UTF8);
        return sign(uuid).equals(token) ? uuid : null;
    }

    // ------------------------------------------------------------------ per request

    private static final class Ctx {
        Req req;
        Map<String, Object> user;
        JSONObject body() {
            try { return new JSONObject(new String(req.body, HttpServer.UTF8)); } catch (Exception e) { return new JSONObject(); }
        }
    }

    private static final class Abort extends RuntimeException {
        final int status;
        Abort(int status) { this.status = status; }
    }

    private static void abort(int status) { throw new Abort(status); }

    private void requireUser(Ctx c) { if (c.user == null) abort(401); }

    private Map<String, Object> getOrCreateUser(String uuid, String deviceId) {
        long t = now();
        List<Map<String, Object>> r = rows("SELECT * FROM users WHERE uuid = ?", uuid);
        if (r.isEmpty()) {
            String invite = UUID.randomUUID().toString().replace("-", "").substring(0, 8).toUpperCase(Locale.US);
            db.execSQL("INSERT INTO users (uuid, device_id, invite_id, crystals, gold_tickets, tickets, tickets_at, created_at, last_seen)"
                    + " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)", new Object[]{uuid, deviceId, invite,
                    S.optInt("new_player_crystals"), S.optInt("new_player_gold_tickets"), S.optInt("ticket_max"), t, t, t});
            for (JSONObject h : heroines)
                db.execSQL("INSERT INTO costumes VALUES (?, ?, ?, 1)", new Object[]{uuid, id(h), S.optString("default_costume")});
            Log.i(TAG, "new player " + uuid + " (invite id " + invite + ")");
        } else {
            db.execSQL("UPDATE users SET last_seen = ? WHERE uuid = ?", new Object[]{t, uuid});
        }
        return rows("SELECT * FROM users WHERE uuid = ?", uuid).get(0);
    }

    private void reload(Ctx c) { c.user = rows("SELECT * FROM users WHERE uuid = ?", Str(c.user, "uuid")).get(0); }

    private static String id(JSONObject heroineEntry) { return heroineEntry.optJSONObject("base").optString("id"); }

    synchronized Resp handle(Req req) {
        Ctx c = new Ctx();
        c.req = req;
        String auth = null, device = null;
        String token = req.header("X-Yoga-Authorization");
        if (token != null) {
            try {
                JSONObject a = new JSONObject(decrypt(token));
                auth = a.optString("uuid", null);
                device = a.optString("device_id", null);
            } catch (Exception e) {
                Log.w(TAG, "could not decrypt X-Yoga-Authorization: " + e);
            }
        }
        if (auth == null) {
            String sess = req.header("X-Mock-Session");
            if (sess == null) sess = req.query.get("_s");
            if (sess != null) auth = unsign(sess);
        }
        if (auth == null && req.query.get("dev") != null) {
            String d = req.query.get("dev").replaceAll("\\W", "");
            auth = "dev-" + d.substring(0, Math.min(32, d.length()));
            device = "browser";
        }
        if (auth != null && !auth.isEmpty()) c.user = getOrCreateUser(auth, device);
        Log.i(TAG, req.method + " " + req.path + (req.rawQuery.isEmpty() ? "" : "?" + req.rawQuery));
        try {
            return route(c);
        } catch (Abort a) {
            if (a.status == 401) return Resp.text(401, "Unauthorized");
            return Resp.text(a.status, a.status == 404 ? "Not Found" : "Bad Request");
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }
    }

    private static final Pattern P_HEROINE = Pattern.compile("^/api/(heroine|stories|gacha|dress_bg|dress_face|costume)/([^/]+)$");
    private static final Pattern P_READ = Pattern.compile("^/api/story/([^/]+)/read$");
    private static final Pattern P_EXCHANGE = Pattern.compile("^/api/shop/exchange/([^/]+)$");
    private static final Pattern P_SCENARIO = Pattern.compile("^/scenarios/([^/]+)/(transcript|resource_list)$");
    private static final Pattern P_NATIVE = Pattern.compile("^/native/([^/]+)$");
    private static final Pattern P_SID = Pattern.compile("[0-9A-Za-z_-]{1,32}");

    private Resp route(Ctx c) throws JSONException {
        Req r = c.req;
        String p = r.path, m = r.method;
        boolean post = m.equals("POST"), get = m.equals("GET") || m.equals("HEAD");
        if (p.equals("/") && get) return index(c);
        if (p.equals("/my/") && get) {
            if (c.user == null) { Resp x = proxyJson(null, "error", "unauthorized"); x.status = 401; return x; }
            return proxyJson(jo("invite_id", Str(c.user, "invite_id")));
        }
        if (p.equals("/api/me") && get) { requireUser(c); return json(playerState(c)); }
        if (p.equals("/api/login_bonus") && post) return loginBonus(c);
        if (p.equals("/api/events") && get) { requireUser(c); return json(jo("events", S.getJSONArray("events"))); }
        if (p.equals("/api/energy/recover") && post) return recoverEnergy(c);
        if (p.equals("/api/panels/use_ticket") && post) return usePanelTicket(c);
        if (p.equals("/api/name") && post) return setName(c);
        if (p.equals("/api/news") && get) return json(jo("news", S.getJSONArray("news")));
        if (p.equals("/api/courses") && get) return json(jo("courses", S.getJSONObject("courses")));
        if (p.equals("/api/soundtrack") && get) return json(jo("tracks", S.getJSONArray("soundtrack")));
        Matcher mm = P_HEROINE.matcher(p);
        if (mm.matches()) {
            String kind = mm.group(1), hid = mm.group(2);
            if (kind.equals("stories") && get) return storiesFor(c, hid);
            if (post) {
                switch (kind) {
                    case "heroine": return selectHeroine(c, hid);
                    case "gacha": return gacha(c, hid);
                    case "dress_bg": return dressBg(c, hid);
                    case "dress_face": return dressFace(c, hid);
                    case "costume": return equip(c, hid);
                }
            }
        }
        mm = P_READ.matcher(p);
        if (mm.matches() && post) return readStory(c, mm.group(1));
        mm = P_EXCHANGE.matcher(p);
        if (mm.matches() && post) return exchange(c, mm.group(1));
        if (p.equals("/resource/character_offset") && get) {
            JSONObject o = readJsonAsset("server/character_offset.json");
            return proxyJson(o != null ? o : jo("01", new JSONObject()));
        }
        mm = P_SCENARIO.matcher(p);
        if (mm.matches() && get) {
            String sid = mm.group(1);
            if (!P_SID.matcher(sid).matches()) abort(400);
            JSONObject content = readJsonAsset("server/scenarios/" + sid + "/" + mm.group(2) + ".json");
            if (mm.group(2).equals("transcript")) return proxyJson(withPlayerName(c, content != null ? content : defaultTranscript(sid)));
            if (content == null) content = jo("resources_id", sid, "resources", new JSONArray(), "next_scenario_id_list", new JSONArray());
            return proxyJson(content, "static_hostname", siteUrl(r) + "/static", "static_version", S.optInt("static_version", 1));
        }
        if (p.equals("/debug/main_story/list") && get) return proxyJson(S.getJSONArray("main_story_list"));
        if ((p.equals("/payment/android/purchase") || p.equals("/payment/android/subscribe")) && post) return paymentPost(c);
        if (p.equals("/native/purchase") && get) {
            JSONObject args = new JSONObject();
            for (Map.Entry<String, String> e : r.query.entrySet()) args.put(e.getKey(), e.getValue());
            grant(c, r.query.containsKey("product_id") ? r.query.get("product_id") : "", args.toString());
            return Resp.of(200, "text/html; charset=utf-8", "__SUCCESS__".getBytes(HttpServer.UTF8));
        }
        mm = P_NATIVE.matcher(p);
        if (mm.matches() && get) return Resp.of(200, "text/html; charset=utf-8", nativeStub(mm.group(1)).getBytes(HttpServer.UTF8));
        // Catch-all (app.py unknown()): top-level page loads from Volley carry the Yoga header -> the SPA.
        Log.w(TAG, "    UNHANDLED " + m + " " + p);
        String accept = r.header("Accept");
        if (get && r.header("X-Yoga-Authorization") != null && (accept == null || !accept.contains("json"))) return index(c);
        Resp x = proxyJson(null, "error", "unhandled path " + p);
        x.status = 404;
        return x;
    }

    private static String siteUrl(Req r) {
        String host = r.header("Host");
        return "http://" + (host != null ? host : "127.0.0.1:" + HttpServer.PORT);
    }

    private String nativeStub(String name) {
        switch (name) {
            case "get_api_version": return String.valueOf(S.optInt("api_version", 5));
            case "get_app_info": return "{\"package_name\": \"jp.mfapps.novel.roomies\", \"version_code\": 0, \"version_name\": \"browser\"}";
            case "get": case "get_price": return "";
            case "get_purchase_data": return "[]";
            case "noah_offer_enable": case "tapjoy_offer_enable": return "false";
            default: return "__SUCCESS__";
        }
    }

    // ------------------------------------------------------------------ front-end page

    private Resp index(Ctx c) throws JSONException {
        requireUser(c);
        // Jinja's tojson escapes these for a <script> block
        String state = playerState(c).toString().replace("<", "\\u003c").replace(">", "\\u003e")
                .replace("&", "\\u0026").replace("'", "\\u0027");
        String html = indexHtml.replace("{{ session }}", sign(Str(c.user, "uuid"))).replace("{{ state | tojson }}", state);
        return Resp.html(html);
    }

    // ------------------------------------------------------------------ tickets

    private long regenSeconds(Map<String, Object> u) {
        return "delux".equals(Str(u, "course")) ? S.optLong("delux_regen") : S.optLong("ticket_regen");
    }

    private long ticketMax(Map<String, Object> u) {
        String course = Str(u, "course");
        long streak = L(u, "login_streak"), cap = S.optLong("ticket_max");
        JSONArray caps = S.optJSONArray("streak_caps");
        for (int i = 0; i < caps.length(); i++) {
            JSONArray dc = caps.optJSONArray(i);
            if (streak >= dc.optLong(0)) { cap = dc.optLong(1); break; }
        }
        long special = "special".equals(course) || "delux".equals(course) ? S.optLong("special_ticket_max") : S.optLong("ticket_max");
        return Math.max(cap, special);
    }

    /** {tickets, max, next_in, anchor} like ticket_state() */
    private long[] ticketState(Map<String, Object> u) {
        long t = now(), regen = regenSeconds(u), top = ticketMax(u);
        long tickets = L(u, "tickets"), anchor = L(u, "tickets_at");
        if (tickets >= top) return new long[]{tickets, top, 0, t};
        long d = t - anchor, gained = d >= 0 ? d / regen : -((-d + regen - 1) / regen);
        tickets = Math.min(top, tickets + gained);
        anchor = tickets >= top ? t : anchor + gained * regen;
        long next = tickets >= top ? 0 : regen - (t - anchor);
        return new long[]{tickets, top, next, anchor};
    }

    private void saveTickets(Ctx c, long tickets, long anchor) {
        db.execSQL("UPDATE users SET tickets = ?, tickets_at = ? WHERE uuid = ?", new Object[]{tickets, anchor, Str(c.user, "uuid")});
    }

    private void addEnergy(Ctx c, long n) {
        long[] t = ticketState(c.user);
        long total = Math.min(t[1], t[0] + n);
        saveTickets(c, total, total >= t[1] ? now() : t[3]);
    }

    // ------------------------------------------------------------------ player state

    private Set<String> readIds(String uuid) {
        Set<String> s = new HashSet<>();
        for (Map<String, Object> r : rows("SELECT scenario_id FROM story_read WHERE uuid = ?", uuid)) s.add(Str(r, "scenario_id"));
        return s;
    }

    private Map<String, Double> progress(Set<String> read) {
        Map<String, Double> out = new LinkedHashMap<>();
        for (JSONObject h : heroines) {
            JSONArray ids = h.optJSONArray("main_ids");
            int done = 0;
            for (int i = 0; i < ids.length(); i++) if (read.contains(ids.optString(i))) done++;
            out.put(id(h), ids.length() == 0 ? 0.0 : Math.round(1000.0 * done / ids.length()) / 10.0);
        }
        return out;
    }

    private String standUrl(JSONObject h, String costume) {
        JSONObject t = h.optJSONObject("stand");
        Object v = costume != null && t.has(costume) ? t.opt(costume) : t.opt("");
        return v == null || v == JSONObject.NULL ? null : v.toString();
    }

    private JSONObject playerState(Ctx c) throws JSONException {
        Map<String, Object> u = c.user;
        String uuid = Str(u, "uuid");
        Set<String> read = readIds(uuid);
        long[] tickets = ticketState(u);
        Map<String, String> equipped = new HashMap<>();
        for (Map<String, Object> r : rows("SELECT heroine_id, costume FROM costumes WHERE uuid = ? AND equipped = 1", uuid))
            equipped.put(Str(r, "heroine_id"), Str(r, "costume"));
        Map<String, JSONArray> owned = new HashMap<>();
        String dflt = S.optString("default_costume"), suffix = S.optString("card_suffix");
        for (Map<String, Object> r : rows("SELECT heroine_id, costume FROM costumes WHERE uuid = ?", uuid)) {
            String hid = Str(r, "heroine_id"), cos = Str(r, "costume");
            JSONObject h = heroineById.get(hid);
            if (h == null) continue;
            if (cos.equals(dflt) || contains(h.optJSONArray("gacha_pool"), cos)) {
                JSONArray a = owned.get(hid);
                if (a == null) owned.put(hid, a = new JSONArray());
                a.put(cos);
                if (contains(h.optJSONArray("card_art"), cos)) a.put(cos + suffix);
            }
        }
        Map<String, Double> prog = progress(read);
        Map<String, String> bgs = new HashMap<>();
        for (Map<String, Object> r : rows("SELECT heroine_id, bg FROM dress_bg WHERE uuid = ?", uuid))
            if (backgrounds.get(Str(r, "bg")) != null) bgs.put(Str(r, "heroine_id"), Str(r, "bg"));
        Map<String, String> faces = new HashMap<>();
        for (Map<String, Object> r : rows("SELECT heroine_id, face FROM dress_face WHERE uuid = ?", uuid))
            faces.put(Str(r, "heroine_id"), Str(r, "face"));
        boolean othersDone = true;
        for (Map.Entry<String, Double> e : prog.entrySet()) if (!e.getKey().equals("06") && e.getValue() < 100) othersDone = false;
        boolean misakiOpen = (prog.containsKey("06") && prog.get("06") > 0) || othersDone;
        JSONArray hs = new JSONArray();
        JSONObject overlays = S.optJSONObject("overlays");
        String defaultBg = S.optString("default_bg"), defaultFace = S.optString("default_face");
        for (JSONObject h : heroines) {
            String hid = id(h);
            JSONObject base = h.optJSONObject("base");
            JSONObject o = new JSONObject(base.toString());
            String cos = equipped.get(hid), face = faces.containsKey(hid) ? faces.get(hid) : defaultFace;
            String bg = bgs.containsKey(hid) ? bgs.get(hid) : defaultBg;
            o.put("progress", prog.get(hid));
            o.put("costume", nul(cos));
            o.put("stand_url", nul(standUrl(h, cos)));
            o.put("face_url", h.opt("face_url"));
            o.put("touch_id", h.opt("touch_id"));
            o.put("touch_line", h.opt("touch_line"));
            o.put("chat", h.opt("chat"));
            o.put("costume_pool", h.opt("costume_pool"));
            o.put("bg", bg);
            o.put("face", face);
            o.put("face_pool", h.opt("face_pool"));
            Object ov = face.equals(defaultFace) || cos == null ? null : overlays.opt(hid + "|" + cos + "|" + face);
            o.put("face_overlay", nul(ov));
            o.put("bg_url", nul(backgrounds.get(bg)));
            o.put("popup_url", h.opt("popup_url"));
            o.put("costumes", owned.containsKey(hid) ? owned.get(hid) : new JSONArray());
            o.put("hidden", hid.equals("06") && !misakiOpen);
            hs.put(o);
        }
        long done = L(u, "pictures_done");
        JSONArray photos = new JSONArray();
        for (int i = 0; i < Math.min(done, panelPhotos.size()); i++) photos.put(panelPhotos.get(i).opt("entry"));
        for (JSONObject p : eventPhotos) photos.put(p.opt("entry"));
        for (JSONObject p : epiloguePhotos) if (read.contains(p.optString("needs"))) photos.put(p.opt("entry"));
        JSONArray bgList = new JSONArray();
        for (Map.Entry<String, String> e : backgrounds.entrySet()) bgList.put(jo("name", e.getKey(), "url", e.getValue()));
        String course = Str(u, "course");
        Object passUntil = u.get("pass_until");
        boolean memoryPass = (course != null && !course.isEmpty() && !course.equals("none")) || (passUntil != null && L(u, "pass_until") > now());
        String prefix = S.optString("story_prefix");
        return jo("invite_id", Str(u, "invite_id"), "name", Str(u, "name"), "heroine_id", Str(u, "heroine_id"),
                "crystals", L(u, "crystals"),
                "items", jo("gold_ticket", L(u, "gold_tickets"), "silver_ticket", L(u, "silver_tickets"),
                        "bronze_ticket", L(u, "bronze_tickets"), "panel_ticket_dx", L(u, "panel_tickets")),
                "course", course, "memory_pass", memoryPass, "pass_until", passUntil,
                "tickets", jo("tickets", tickets[0], "max", tickets[1], "next_in", tickets[2]),
                "login_streak", L(u, "login_streak"),
                "panels", jo("revealed", L(u, "panels"), "total", S.optInt("panels_per_picture"), "completed", done,
                        "current", panelPhotos.isEmpty() ? null : panelPhotos.get((int) (done % panelPhotos.size())).opt("entry")),
                "photos", photos,
                "prologue_done", read.contains(prefix + "0000000002"),
                "heroines", hs, "backgrounds", bgList);
    }

    private static boolean contains(JSONArray a, String s) {
        if (a == null) return false;
        for (int i = 0; i < a.length(); i++) if (s.equals(a.optString(i))) return true;
        return false;
    }

    // ------------------------------------------------------------------ API

    private static String today(int offsetDays) {
        Calendar cal = Calendar.getInstance();
        cal.add(Calendar.DAY_OF_YEAR, offsetDays);
        return new SimpleDateFormat("yyyy-MM-dd", Locale.US).format(cal.getTime());
    }

    private Resp loginBonus(Ctx c) throws JSONException {
        requireUser(c);
        String today = today(0);
        if (today.equals(Str(c.user, "last_login_day"))) return json(jo("granted", false, "state", playerState(c)));
        long panels = L(c.user, "panels") + 1, done = L(c.user, "pictures_done");
        if (panels >= S.optInt("panels_per_picture")) { panels = 0; done++; }
        long streak = today(-1).equals(Str(c.user, "last_login_day")) ? L(c.user, "login_streak") + 1 : 1;
        String uuid = Str(c.user, "uuid");
        db.execSQL("UPDATE users SET panels = ?, pictures_done = ?, last_login_day = ?, login_streak = ?, login_days = login_days + 1"
                + " WHERE uuid = ?", new Object[]{panels, done, today, streak, uuid});
        long days = L(c.user, "login_days") + 1;
        String item = null;
        if (days % 2 == 0) {
            item = days % 4 == 2 ? "silver_ticket" : "panel_ticket_dx";
            String col = item.equals("silver_ticket") ? "silver_tickets" : "panel_tickets";
            db.execSQL("UPDATE users SET " + col + " = " + col + " + 1 WHERE uuid = ?", new Object[]{uuid});
        }
        reload(c);
        return json(jo("granted", true, "item", item, "next_in", days % 2 != 0 ? 1 : 2, "state", playerState(c)));
    }

    private Resp selectHeroine(Ctx c, String hid) throws JSONException {
        requireUser(c);
        if (!heroineById.containsKey(hid)) abort(404);
        db.execSQL("UPDATE users SET heroine_id = ? WHERE uuid = ?", new Object[]{hid, Str(c.user, "uuid")});
        reload(c);
        return json(playerState(c));
    }

    private Resp storiesFor(Ctx c, String hid) throws JSONException {
        requireUser(c);
        if (!heroineById.containsKey(hid)) abort(404);
        Set<String> read = readIds(Str(c.user, "uuid"));
        JSONArray out = new JSONArray();
        for (JSONObject s : stories) {
            String h = s.optString("heroine_id");
            if (!h.equals("00") && !h.equals(hid)) continue;
            JSONObject o = new JSONObject(s.toString());
            Object prev = o.remove("prev");
            String sid = s.optString("scenario_id");
            boolean isRead = read.contains(sid);
            boolean locked = prev != null && prev != JSONObject.NULL && !read.contains(prev.toString()) && !isRead;
            o.put("read", isRead);
            o.put("locked", locked);
            out.put(o);
        }
        return json(jo("stories", out));
    }

    private Resp readStory(Ctx c, String sid) throws JSONException {
        if (!P_SID.matcher(sid).matches()) abort(400);
        requireUser(c);
        JSONObject story = storyById.get(sid);
        if (story == null) abort(404);
        String uuid = Str(c.user, "uuid");
        Set<String> read = readIds(uuid);
        if (!read.contains(sid)) {
            Object prev = story.opt("prev");
            if (prev != null && prev != JSONObject.NULL && !read.contains(prev.toString()))
                return json(403, jo("error", "locked", "message", "Read the previous part first."));
            String kind = story.optString("kind");
            if (kind.equals("epilogue") || kind.equals("secret")) {
                long cost = kind.equals("epilogue") ? S.optLong("epilogue_cost") : S.optLong("secret_cost");
                long have = L(c.user, "crystals");
                if (have < cost) {
                    String what = kind.equals("epilogue") ? "The epilogue" : "\"" + story.optString("title") + "\"";
                    return json(402, jo("error", "crystals", "message", what + " costs " + cost + " crystals. You need "
                            + (cost - have) + " crystal(s) more!"));
                }
                db.execSQL("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", new Object[]{cost, uuid});
                db.execSQL("INSERT OR IGNORE INTO story_read VALUES (?, ?, ?)", new Object[]{uuid, sid, now()});
                reload(c);
                return json(jo("ok", true, "state", playerState(c)));
            }
            long[] t = ticketState(c.user);
            if (t[0] < 1) return json(409, jo("error", "no_energy", "state", playerState(c)));
            saveTickets(c, t[0] - 1, t[0] < t[1] ? t[3] : now());
        }
        db.execSQL("INSERT OR IGNORE INTO story_read VALUES (?, ?, ?)", new Object[]{uuid, sid, now()});
        reload(c);
        return json(jo("ok", true, "state", playerState(c)));
    }

    private Resp recoverEnergy(Ctx c) throws JSONException {
        requireUser(c);
        String method = c.body().optString("method", "gold_ticket");
        Map<String, Object> u = c.user;
        String uuid = Str(u, "uuid");
        switch (method) {
            case "crystal":
                if (L(u, "crystals") < S.optLong("crystals_per_energy")) return json(402, jo("error", "not enough crystals"));
                db.execSQL("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", new Object[]{S.optLong("crystals_per_energy"), uuid});
                addEnergy(c, 1);
                break;
            case "gold_ticket":
                if (L(u, "gold_tickets") < 1) return json(402, jo("error", "no Gold Tickets"));
                db.execSQL("UPDATE users SET gold_tickets = gold_tickets - 1 WHERE uuid = ?", new Object[]{uuid});
                saveTickets(c, ticketMax(u), now());
                break;
            case "silver_ticket":
                if (L(u, "silver_tickets") < 1) return json(402, jo("error", "no Silver Tickets"));
                db.execSQL("UPDATE users SET silver_tickets = silver_tickets - 1 WHERE uuid = ?", new Object[]{uuid});
                addEnergy(c, 1);
                break;
            case "bronze_ticket":
                long need = S.optLong("bronze_per_energy");
                if (L(u, "bronze_tickets") < need) return json(402, jo("error", "need " + need + " Bronze Tickets"));
                db.execSQL("UPDATE users SET bronze_tickets = bronze_tickets - ? WHERE uuid = ?", new Object[]{need, uuid});
                addEnergy(c, 1);
                break;
            default:
                abort(400);
        }
        reload(c);
        return json(jo("ok", true, "state", playerState(c)));
    }

    private Resp exchange(Ctx c, String item) throws JSONException {
        requireUser(c);
        JSONObject prices = S.optJSONObject("item_prices"), days = S.optJSONObject("pass_days");
        if (!prices.has(item)) abort(404);
        long price = prices.optLong(item);
        if (L(c.user, "crystals") < price) return json(402, jo("error", "not enough crystals"));
        String uuid = Str(c.user, "uuid");
        if (days.has(item)) {
            long start = Math.max(now(), L(c.user, "pass_until"));
            db.execSQL("UPDATE users SET crystals = crystals - ?, pass_until = ? WHERE uuid = ?",
                    new Object[]{price, start + days.optLong(item) * 86400, uuid});
        } else {
            String col = item.equals("gold_ticket") ? "gold_tickets" : "panel_tickets";
            db.execSQL("UPDATE users SET crystals = crystals - ?, " + col + " = " + col + " + 1 WHERE uuid = ?", new Object[]{price, uuid});
        }
        reload(c);
        return json(jo("ok", true, "state", playerState(c)));
    }

    private Resp usePanelTicket(Ctx c) throws JSONException {
        requireUser(c);
        if (L(c.user, "panel_tickets") < 1) return json(402, jo("error", "no Panel Ticket DX"));
        long panels = L(c.user, "panels") + 1, done = L(c.user, "pictures_done");
        if (panels >= S.optInt("panels_per_picture")) { panels = 0; done++; }
        db.execSQL("UPDATE users SET panel_tickets = panel_tickets - 1, panels = ?, pictures_done = ? WHERE uuid = ?",
                new Object[]{panels, done, Str(c.user, "uuid")});
        reload(c);
        return json(jo("ok", true, "state", playerState(c)));
    }

    private Resp setName(Ctx c) throws JSONException {
        requireUser(c);
        String name = c.body().optString("name", "").trim();
        if (name.length() > 12) name = name.substring(0, 12);
        if (name.isEmpty()) abort(400);
        db.execSQL("UPDATE users SET name = ? WHERE uuid = ?", new Object[]{name, Str(c.user, "uuid")});
        reload(c);
        return json(playerState(c));
    }

    private Resp gacha(Ctx c, String hid) throws JSONException {
        requireUser(c);
        JSONObject h = heroineById.get(hid);
        if (h == null) abort(404);
        JSONArray pool = h.optJSONArray("gacha_pool");
        if (pool == null || pool.length() == 0)
            return json(404, jo("error", "No outfits for " + h.optJSONObject("base").optString("short") + " have been recovered yet."));
        long cost = S.optLong("gacha_cost");
        if (L(c.user, "crystals") < cost) return json(402, jo("error", "not enough crystals"));
        String costume = pool.optString(random.nextInt(pool.length()));
        String uuid = Str(c.user, "uuid");
        db.execSQL("UPDATE users SET crystals = crystals - ? WHERE uuid = ?", new Object[]{cost, uuid});
        ContentValues v = new ContentValues();
        v.put("uuid", uuid); v.put("heroine_id", hid); v.put("costume", costume); v.put("equipped", 0);
        boolean duplicate = db.insertWithOnConflict("costumes", null, v, SQLiteDatabase.CONFLICT_IGNORE) == -1;
        if (duplicate) db.execSQL("UPDATE users SET gold_tickets = gold_tickets + 1 WHERE uuid = ?", new Object[]{uuid});
        reload(c);
        return json(jo("costume", costume, "duplicate", duplicate, "url", standUrl(h, costume), "state", playerState(c)));
    }

    private Resp dressBg(Ctx c, String hid) throws JSONException {
        requireUser(c);
        String bg = c.body().optString("bg", null);
        if (!heroineById.containsKey(hid) || bg == null || backgrounds.get(bg) == null) abort(404);
        db.execSQL("INSERT OR REPLACE INTO dress_bg (uuid, heroine_id, bg) VALUES (?, ?, ?)", new Object[]{Str(c.user, "uuid"), hid, bg});
        return json(playerState(c));
    }

    private Resp dressFace(Ctx c, String hid) throws JSONException {
        requireUser(c);
        String face = c.body().optString("face", null);
        JSONObject h = heroineById.get(hid);
        if (h == null || face == null || (!face.equals(S.optString("default_face")) && !contains(h.optJSONArray("faces"), face))) abort(404);
        db.execSQL("INSERT OR REPLACE INTO dress_face (uuid, heroine_id, face) VALUES (?, ?, ?)", new Object[]{Str(c.user, "uuid"), hid, face});
        return json(playerState(c));
    }

    private Resp equip(Ctx c, String hid) throws JSONException {
        requireUser(c);
        String costume = c.body().optString("costume", null);
        if (costume == null) abort(404);
        JSONObject h = heroineById.get(hid);
        String suffix = S.optString("card_suffix"), base = null;
        if (h != null && costume.endsWith(suffix)) {
            String b = costume.substring(0, costume.length() - suffix.length());
            if (contains(h.optJSONArray("card_art"), b)) base = b;
        }
        String uuid = Str(c.user, "uuid");
        if (scalar("SELECT 1 FROM costumes WHERE uuid = ? AND heroine_id = ? AND costume = ?", uuid, hid, base != null ? base : costume) == null)
            abort(404);
        if (base != null) db.execSQL("INSERT OR IGNORE INTO costumes VALUES (?, ?, ?, 0)", new Object[]{uuid, hid, costume});
        db.execSQL("UPDATE costumes SET equipped = (costume = ?) WHERE uuid = ? AND heroine_id = ?", new Object[]{costume, uuid, hid});
        return json(playerState(c));
    }

    // ------------------------------------------------------------------ story engine, payments

    private JSONObject defaultTranscript(String sid) throws JSONException {
        JSONArray data = new JSONArray();
        data.put(jo("scene", jo("speaker", "__NULL__", "body", "(This part's text hasn't been recovered yet.)",
                "show_tail", "none", "link_scene", "0"), "visual", jo("end_mark", jo("type", "continue"))));
        return jo("scenario_id", sid, "scenario_data", data);
    }

    private static final Pattern P_WILLIAM = Pattern.compile("\\bWilliam\\b");

    /** app.py with_player_name(): the chosen name (More > Name) in place of William in the speaker box and text. */
    private JSONObject withPlayerName(Ctx c, JSONObject content) throws JSONException {
        String name = c.user == null ? null : Str(c.user, "name");
        if (name == null || name.isEmpty() || name.equals("William")) return content;
        String rep = Matcher.quoteReplacement(name);
        JSONArray data = content.optJSONArray("scenario_data");
        for (int i = 0; data != null && i < data.length(); i++) {
            JSONObject scene = data.getJSONObject(i).optJSONObject("scene");
            if (scene == null) continue;
            for (String k : new String[]{"speaker", "body"}) {
                Object v = scene.opt(k);
                if (v instanceof String) scene.put(k, P_WILLIAM.matcher((String) v).replaceAll(rep));
            }
        }
        return content;
    }

    private static final Pattern P_CREDIT = Pattern.compile("\\.credit\\.(\\d+)");

    private void grant(Ctx c, String productId, String raw) {
        if (c.user == null) return;
        Matcher m = P_CREDIT.matcher(productId == null ? "" : productId);
        long n = m.find() ? Long.parseLong(m.group(1)) : 0;
        String uuid = Str(c.user, "uuid");
        db.execSQL("UPDATE users SET crystals = crystals + ? WHERE uuid = ?", new Object[]{n, uuid});
        db.execSQL("INSERT INTO purchases (uuid, product_id, raw, created_at) VALUES (?, ?, ?, ?)", new Object[]{uuid, productId, raw, now()});
    }

    private Resp paymentPost(Ctx c) throws JSONException {
        String token = c.req.form().get("purchase_data");
        JSONObject purchase;
        try {
            purchase = new JSONObject(decrypt(token == null ? "" : token));
        } catch (Exception e) {
            Log.w(TAG, "bad purchase_data: " + e);
            return json(400, jo("error", "bad purchase_data"));
        }
        grant(c, purchase.optString("productId", null), purchase.toString());
        return json(jo("result", "ok"));
    }

    @SuppressWarnings("unused")
    private static Iterator<String> keys(JSONObject o) { return o.keys(); }
}
