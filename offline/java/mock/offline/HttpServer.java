package mock.offline;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URLDecoder;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * A minimal HTTP/1.1 server on 127.0.0.1 (one request per connection), the in-app stand-in for server/app.py.
 * Game requests go to {@link Game}; /static/ files come from the APK's assets (assets/server/static).
 */
public final class HttpServer {
    static final String TAG = "RoomiesMock";
    static final int PORT = 18765;                  // must match site_url in the patched app.json
    private static volatile boolean started;

    static Context ctx;
    static AssetManager assets;
    static File pngCache;
    private static Game game;
    private static final Map<String, Boolean> exists = new ConcurrentHashMap<>();

    public static synchronized void start(Context c) throws IOException {
        if (started) return;
        ctx = c.getApplicationContext() != null ? c.getApplicationContext() : c;
        assets = ctx.getAssets();
        pngCache = new File(ctx.getCacheDir(), "mock_png");
        try { game = new Game(ctx); } catch (org.json.JSONException e) { throw new IOException(e); }
        final ServerSocket ss = new ServerSocket(PORT, 64, InetAddress.getByName("127.0.0.1"));
        ss.setReuseAddress(true);
        Thread t = new Thread(new Acceptor(ss, Executors.newFixedThreadPool(8)), "mock-server");
        t.setDaemon(true);
        t.start();
        started = true;
        Log.i(TAG, "listening on 127.0.0.1:" + PORT);
    }

    private static final class Acceptor implements Runnable {
        private final ServerSocket ss;
        private final ExecutorService pool;
        Acceptor(ServerSocket ss, ExecutorService pool) { this.ss = ss; this.pool = pool; }
        public void run() {
            while (true) {
                try {
                    pool.execute(new Worker(ss.accept()));
                } catch (Throwable e) {
                    Log.e(TAG, "accept", e);
                }
            }
        }
    }

    private static final class Worker implements Runnable {
        private final Socket s;
        Worker(Socket s) { this.s = s; }
        public void run() { serve(s); }
    }

    /** One parsed request. Header names are lower case. */
    static final class Req {
        String method, path, rawQuery = "";
        final Map<String, String> query = new HashMap<>(), headers = new HashMap<>();
        byte[] body = new byte[0];

        String header(String name) { return headers.get(name.toLowerCase(Locale.US)); }

        Map<String, String> form() {
            Map<String, String> m = new HashMap<>();
            parseQuery(new String(body, UTF8), m);
            return m;
        }
    }

    static final class Resp {
        int status = 200;
        String type = "application/json";
        byte[] body = new byte[0];
        Map<String, String> extra;

        static Resp of(int status, String type, byte[] body) {
            Resp r = new Resp(); r.status = status; r.type = type; r.body = body; return r;
        }
        static Resp text(int status, String s) { return of(status, "text/plain; charset=utf-8", s.getBytes(UTF8)); }
        static Resp json(int status, String s) { return of(status, "application/json", s.getBytes(UTF8)); }
        static Resp html(String s) { return of(200, "text/html; charset=utf-8", s.getBytes(UTF8)); }
    }

    static final java.nio.charset.Charset UTF8 = java.nio.charset.Charset.forName("UTF-8");

    private static void serve(Socket s) {
        try {
            s.setSoTimeout(30000);
            InputStream in = new BufferedInputStream(s.getInputStream());
            Req req = parse(in);
            if (req == null) { s.close(); return; }
            Resp resp;
            try {
                long t0 = System.currentTimeMillis();
                resp = req.path.startsWith("/static/") ? staticFile(req) : game.handle(req);
                if (req.path.startsWith("/static/"))
                    Log.i(TAG, req.method + " " + req.path + " -> " + resp.status + " " + resp.body.length + "B "
                            + (System.currentTimeMillis() - t0) + "ms range=" + req.header("range") + " ua=" + req.header("user-agent"));
            } catch (Throwable e) {
                Log.e(TAG, req.method + " " + req.path, e);
                resp = Resp.text(500, "Internal Server Error");
            }
            write(s.getOutputStream(), req, resp);
        } catch (Throwable e) {
            Log.w(TAG, "connection: " + e);
        } finally {
            try { s.close(); } catch (IOException ignored) { }
        }
    }

    private static String readLine(InputStream in) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        int c;
        while ((c = in.read()) != -1) {
            if (c == '\n') break;
            if (c != '\r') b.write(c);
            if (b.size() > 65536) throw new IOException("line too long");
        }
        if (c == -1 && b.size() == 0) return null;
        return new String(b.toByteArray(), UTF8);
    }

    private static Req parse(InputStream in) throws IOException {
        String line = readLine(in);
        if (line == null || line.isEmpty()) return null;
        String[] parts = line.split(" ");
        if (parts.length < 2) return null;
        Req r = new Req();
        r.method = parts[0];
        String target = parts[1];
        // The native engine's HTTP client (Volley/HttpURLConnection) sends the request line in absolute-form
        // ("GET http://127.0.0.1:18765/my/ HTTP/1.1") on some devices/Android versions, not origin-form ("GET /my/").
        // RFC 7230 5.3.2: a server must accept absolute-form, so strip the scheme+authority down to the path, or every
        // route misses and the request falls through to the SPA index (mypage + all story loading fail).
        if (target.startsWith("http://") || target.startsWith("https://")) {
            int s = target.indexOf("://") + 3, slash = target.indexOf('/', s);
            target = slash >= 0 ? target.substring(slash) : "/";
        }
        int q = target.indexOf('?');
        r.path = q >= 0 ? target.substring(0, q) : target;
        if (q >= 0) { r.rawQuery = target.substring(q + 1); parseQuery(r.rawQuery, r.query); }
        try { r.path = URLDecoder.decode(r.path.replace("+", "%2B"), "UTF-8"); } catch (Exception ignored) { }
        String h;
        while ((h = readLine(in)) != null && !h.isEmpty()) {
            int c = h.indexOf(':');
            if (c > 0) r.headers.put(h.substring(0, c).trim().toLowerCase(Locale.US), h.substring(c + 1).trim());
        }
        String len = r.headers.get("content-length");
        if (len != null) {
            int n = Integer.parseInt(len.trim());
            byte[] b = new byte[n];
            int off = 0;
            while (off < n) {
                int k = in.read(b, off, n - off);
                if (k < 0) break;
                off += k;
            }
            r.body = b;
        }
        return r;
    }

    static void parseQuery(String q, Map<String, String> into) {
        if (q == null || q.isEmpty()) return;
        for (String kv : q.split("&")) {
            if (kv.isEmpty()) continue;
            int e = kv.indexOf('=');
            try {
                String k = URLDecoder.decode(e >= 0 ? kv.substring(0, e) : kv, "UTF-8");
                String v = e >= 0 ? URLDecoder.decode(kv.substring(e + 1), "UTF-8") : "";
                if (!into.containsKey(k)) into.put(k, v);
            } catch (Exception ignored) { }
        }
    }

    private static final Map<Integer, String> REASONS = new HashMap<>();
    static {
        REASONS.put(200, "OK"); REASONS.put(206, "Partial Content"); REASONS.put(400, "Bad Request");
        REASONS.put(401, "Unauthorized"); REASONS.put(402, "Payment Required"); REASONS.put(403, "Forbidden");
        REASONS.put(404, "Not Found"); REASONS.put(409, "Conflict"); REASONS.put(416, "Range Not Satisfiable");
        REASONS.put(500, "Internal Server Error");
    }

    private static void write(OutputStream out, Req req, Resp r) throws IOException {
        byte[] body = r.body;
        int status = r.status;
        String range = req.header("range");
        String extraRange = null;
        if (status == 200 && range != null && range.startsWith("bytes=") && body.length > 0) {
            try {
                String[] se = range.substring(6).split("-", -1);
                long a = se[0].isEmpty() ? body.length - Long.parseLong(se[1]) : Long.parseLong(se[0]);
                long b = se[0].isEmpty() || se[1].isEmpty() ? body.length - 1 : Math.min(Long.parseLong(se[1]), body.length - 1);
                if (a >= 0 && a <= b) {
                    byte[] part = new byte[(int) (b - a + 1)];
                    System.arraycopy(body, (int) a, part, 0, part.length);
                    extraRange = "bytes " + a + "-" + b + "/" + body.length;
                    body = part;
                    status = 206;
                }
            } catch (Exception ignored) { }
        }
        StringBuilder sb = new StringBuilder();
        sb.append("HTTP/1.1 ").append(status).append(' ').append(REASONS.containsKey(status) ? REASONS.get(status) : "OK").append("\r\n");
        sb.append("Content-Type: ").append(r.type).append("\r\n");
        sb.append("Content-Length: ").append(body.length).append("\r\n");
        sb.append("Accept-Ranges: bytes\r\n");
        if (extraRange != null) sb.append("Content-Range: ").append(extraRange).append("\r\n");
        if (r.extra != null) for (Map.Entry<String, String> e : r.extra.entrySet()) sb.append(e.getKey()).append(": ").append(e.getValue()).append("\r\n");
        sb.append("Connection: close\r\n\r\n");
        out.write(sb.toString().getBytes(UTF8));
        if (!"HEAD".equals(req.method)) out.write(body);
        out.flush();
    }

    // ------------------------------------------------------------------ assets

    static boolean assetExists(String path) {
        Boolean b = exists.get(path);
        if (b != null) return b;
        boolean ok;
        try { assets.open(path).close(); ok = true; } catch (IOException e) { ok = false; }
        exists.put(path, ok);
        return ok;
    }

    static byte[] readAsset(String path) throws IOException {
        return readAll(assets.open(path));
    }

    static byte[] readAll(InputStream in) throws IOException {
        try {
            ByteArrayOutputStream b = new ByteArrayOutputStream(Math.max(8192, in.available()));
            byte[] buf = new byte[65536];
            int n;
            while ((n = in.read(buf)) > 0) b.write(buf, 0, n);
            return b.toByteArray();
        } finally {
            in.close();
        }
    }

    static String mime(String path) {
        String p = path.toLowerCase(Locale.US);
        if (p.endsWith(".png")) return "image/png";
        if (p.endsWith(".webp")) return "image/webp";
        if (p.endsWith(".jpg") || p.endsWith(".jpeg")) return "image/jpeg";
        if (p.endsWith(".gif")) return "image/gif";
        if (p.endsWith(".ogg")) return "audio/ogg";
        if (p.endsWith(".mp3")) return "audio/mpeg";
        if (p.endsWith(".m4a")) return "audio/mp4";
        if (p.endsWith(".mp4")) return "video/mp4";
        if (p.endsWith(".ttf")) return "font/ttf";
        if (p.endsWith(".otf")) return "font/otf";
        if (p.endsWith(".woff")) return "font/woff";
        if (p.endsWith(".woff2")) return "font/woff2";
        if (p.endsWith(".json")) return "application/json";
        if (p.endsWith(".css")) return "text/css";
        if (p.endsWith(".js")) return "application/javascript";
        if (p.endsWith(".html")) return "text/html; charset=utf-8";
        if (p.endsWith(".zip")) return "application/zip";
        return "application/octet-stream";
    }

    /**
     * /static/<version>/<path>. Pictures are stored as WebP (export_static.py). The WebView gets the WebP bytes as
     * they are (it shows WebP under a .png name); the story engine, which only loads PNG, gets a PNG made once and
     * kept in the app's cache.
     */
    private static Resp staticFile(Req req) throws IOException {
        String p = req.path.substring("/static/".length());
        int slash = p.indexOf('/');
        if (slash < 0) return Resp.text(404, "Not Found");
        String rel = p.substring(slash + 1);
        if (rel.contains("..")) return Resp.text(404, "Not Found");
        String asset = "server/static/" + rel;
        if (assetExists(asset)) return Resp.of(200, mime(rel), readAsset(asset));
        if (rel.toLowerCase(Locale.US).endsWith(".png")) {
            String webp = asset.substring(0, asset.length() - 4) + ".webp";
            if (assetExists(webp)) {
                String ua = req.header("user-agent");
                if (ua != null && ua.contains("Mozilla")) return Resp.of(200, "image/webp", readAsset(webp));
                return Resp.of(200, "image/png", pngFor(rel, webp));
            }
        }
        return Resp.text(404, "Not Found");
    }

    private static byte[] pngFor(String rel, String webpAsset) throws IOException {
        File f = new File(pngCache, rel);
        if (f.isFile() && f.length() > 0) return readAll(new FileInputStream(f));
        byte[] src = readAsset(webpAsset);
        Bitmap bm = BitmapFactory.decodeByteArray(src, 0, src.length);
        if (bm == null) throw new IOException("can't decode " + webpAsset);
        ByteArrayOutputStream b = new ByteArrayOutputStream(src.length * 4);
        bm.compress(Bitmap.CompressFormat.PNG, 100, b);
        bm.recycle();
        byte[] png = b.toByteArray();
        File dir = f.getParentFile();
        if (dir != null && (dir.isDirectory() || dir.mkdirs())) {
            File tmp = new File(dir, f.getName() + ".part" + Thread.currentThread().getId());
            FileOutputStream o = new FileOutputStream(tmp);
            try { o.write(png); } finally { o.close(); }
            if (!tmp.renameTo(f)) tmp.delete();
        }
        return png;
    }
}
