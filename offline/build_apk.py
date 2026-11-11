"""Offline APK, step 3 (2026-10-15): build "My Sweet Roomies! (Offline)" with the mock server inside.

Starting from the decoded original (offline/decoded, apktool 2.10.0):
  1. app.json (assets/Application/document/json/*/android) points site_url / resource_url at http://127.0.0.1:18765
  2. the manifest gets package jp.mfapps.novel.roomies.offline (so it installs next to the original), targetSdk 23
     (so Android 14+ will install it), usesCleartextTraffic (the mock is plain HTTP on localhost), its unused and
     Play-Protect-flagging permissions dropped, and a content provider mock.offline.Boot that starts the server;
     AuthInfo.smali's getDeviceId() call is patched out (it throws on Android 10+ and crashed the loading screen)
  3. apktool rebuilds it; then the Java server (offline/java, compiled to classes2.dex) and assets/server
     (export_data.py, export_static.py) are added, pictures/sound/video stored uncompressed
  4. zipalign, then apksigner with offline/offline.keystore (made on first run)
The server's AES key comes from the APK's own signature at run time, so any signing key works.
usage: python build_apk.py [--debug] [--arm64]   (run export_data.py and export_static.py first)
  --debug: also turn on the engine's AppLog (res bool app_log) and android:debuggable (run-as, chrome://inspect
           for the WebView), written to MySweetRoomies_offline_debug.apk; same package and key as the release
  --arm64: also pack the rewritten engine (native64/, built with CMake) as lib/arm64-v8a/libotome.so, so the APK
           runs on 64-bit-only phones; 64-bit phones that can still run 32-bit code pick it too (Android prefers
           the primary ABI), while 32-bit phones keep the original lib/armeabi build"""
import os
import re
import shutil
import subprocess
import sys
import zipfile

R = os.path.dirname(os.path.abspath(__file__))
SDK = os.path.expandvars(r"%LOCALAPPDATA%\Android\Sdk")
BT = os.path.join(SDK, "build-tools", "34.0.0")
AJ = os.path.join(SDK, "platforms", "android-33", "android.jar")
JDK = r"C:\Program Files\Java\jdk-21\bin"
APKTOOL = os.path.join(R, "tools", "apktool.jar")
ORIG = os.path.join(R, "..", "My+Sweet+Roomies!_1.4.6_APKPure.apk")
WORK = os.path.join(R, "build", "work")
B = os.path.join(R, "build")
OUT = os.path.join(R, "MySweetRoomies_offline.apk")
PKG = "jp.mfapps.novel.roomies.offline"
SITE = "http://127.0.0.1:18765"
STORE_EXT = (".webp", ".png", ".jpg", ".ogg", ".mp3", ".mp4", ".ttf", ".otf", ".woff", ".woff2", ".zip")
TARGET_SDK = 24          # Android raised the install floor over time: 14 refuses targetSdk < 23, and 15/16 refuse
                         # < 24 (INSTALL_FAILED_DEPRECATED_SDK_VERSION on a Galaxy Tab S9 FE, Android 16). 24 is the
                         # lowest that installs everywhere while keeping cleartext on by default (off only at 28+)
                         # and avoiding the exported-component requirement (31+) -- fewest behaviour changes.
# Permissions an offline build never uses; several (INTERACT_ACROSS_USERS_FULL, SYSTEM_ALERT_WINDOW,
# WRITE_SETTINGS, GET_TASKS) are exactly what Play Protect flags on a sideloaded repack, so drop them all.
DROP_PERMS = {
    "android.permission.INTERACT_ACROSS_USERS_FULL",
    "android.permission.SYSTEM_ALERT_WINDOW",
    "android.permission.WRITE_SETTINGS",
    "android.permission.GET_TASKS",
    "android.permission.DISABLE_KEYGUARD",
    "android.permission.GET_ACCOUNTS",
    "android.permission.ACCESS_WIFI_STATE",
    "com.android.vending.BILLING",
    "com.google.android.c2dm.permission.RECEIVE",
    "jp.mfapps.novel.roomies.permission.C2D_MESSAGE",
    # No longer needed once the getDeviceId() call is patched out (see patch_tree); the offline build keeps
    # only INTERNET (localhost socket), ACCESS_NETWORK_STATE, VIBRATE and WAKE_LOCK, all "normal" permissions.
    "android.permission.READ_PHONE_STATE",
    "android.permission.WRITE_EXTERNAL_STORAGE",
}


def run(*cmd, **kw):
    print(">", " ".join(str(c) for c in cmd)[:160], flush=True)
    subprocess.run([str(c) for c in cmd], check=True, **kw)


def compile_java():
    cls = os.path.join(B, "classes")
    shutil.rmtree(cls, ignore_errors=True)
    os.makedirs(cls)
    srcs = [os.path.join(dp, f) for dp, _, fs in os.walk(os.path.join(R, "java")) for f in fs if f.endswith(".java")]
    run(os.path.join(JDK, "javac.exe"), "-source", "8", "-target", "8", "-bootclasspath", AJ, "-Xlint:-options",
        "-encoding", "utf8", "-d", cls, *srcs)
    jar = os.path.join(B, "classes.jar")
    with zipfile.ZipFile(jar, "w") as z:
        for dp, _, fs in os.walk(cls):
            for f in fs:
                z.write(os.path.join(dp, f), os.path.relpath(os.path.join(dp, f), cls).replace(os.sep, "/"))
    dex = os.path.join(B, "dex")
    shutil.rmtree(dex, ignore_errors=True)
    os.makedirs(dex)
    run(os.path.join(JDK, "java.exe"), "-cp", os.path.join(BT, "lib", "d8.jar"), "com.android.tools.r8.D8",
        "--min-api", "14", "--lib", AJ, "--output", dex, jar)
    return os.path.join(dex, "classes.dex")


def patch_tree(debug=False):
    shutil.rmtree(WORK, ignore_errors=True)
    shutil.copytree(os.path.join(R, "decoded"), WORK)
    jdir = os.path.join(WORK, "assets", "Application", "document", "json")
    for dp, _, fs in os.walk(jdir):
        for f in fs:
            if f == "app.json" and dp.endswith("android"):
                p = os.path.join(dp, f)
                s = open(p, encoding="utf8").read()
                s2 = re.sub(r'"(site_url|resource_url)"\s*:\s*"[^"]*"', lambda m: f'"{m.group(1)}": "{SITE}"', s)
                open(p, "w", encoding="utf8").write(s2)
    man = os.path.join(WORK, "AndroidManifest.xml")
    s = open(man, encoding="utf8").read()
    # Strip the unused / Play-Protect-flagging permissions and the custom C2D_MESSAGE <permission> they go with.
    s = re.sub(r' *<uses-permission android:name="([^"]+)"/>\n',
               lambda m: "" if m.group(1) in DROP_PERMS else m.group(0), s)
    s = re.sub(r' *<permission android:name="jp\.mfapps\.novel\.roomies\.permission\.C2D_MESSAGE"[^>]*/>\n', "", s)
    # The in-app mock is plain HTTP on localhost; keep cleartext allowed now that we target API 23.
    s = s.replace("<application ", '<application android:usesCleartextTraffic="true" ', 1)
    if debug:
        s = s.replace("<application ", '<application android:debuggable="true" ', 1)
        bools = os.path.join(WORK, "res", "values", "bools.xml")
        b = open(bools, encoding="utf8").read()
        open(bools, "w", encoding="utf8").write(b.replace('<bool name="app_log">false</bool>', '<bool name="app_log">true</bool>'))
    provider = (f'<provider android:name="mock.offline.Boot" android:authorities="{PKG}.mockserver" '
                'android:exported="false" android:initOrder="1000"/>')
    s = s.replace("</application>", provider + "</application>", 1)
    open(man, "w", encoding="utf8").write(s)
    yml = os.path.join(WORK, "apktool.yml")
    y = open(yml, encoding="utf8").read().replace("renameManifestPackage: null", f"renameManifestPackage: {PKG}")
    y = re.sub(r"targetSdkVersion:\s*\d+", f"targetSdkVersion: {TARGET_SDK}", y)
    open(yml, "w", encoding="utf8").write(y)
    # TelephonyManager.getDeviceId() throws SecurityException on Android 10+ for a non-privileged app, which
    # crashed the app on the WebView's first request ("stuck on the loading screen"). AuthInfo.readDeviceId already
    # falls back to a Build-based id when getDeviceId returns null, so force the null: replace the call + its result
    # with "const/4 v1, 0x0". This works on every phone and needs no READ_PHONE_STATE.
    auth = os.path.join(WORK, "smali", "jp", "mfapps", "framework", "maidengine", "model", "info", "AuthInfo.smali")
    a = open(auth, encoding="utf8").read()
    a2 = a.replace(
        "    invoke-virtual {v2}, Landroid/telephony/TelephonyManager;->getDeviceId()Ljava/lang/String;\n\n"
        "    move-result-object v1\n",
        "    const/4 v1, 0x0\n")
    if a2 == a:
        raise SystemExit("AuthInfo getDeviceId patch did not apply -- the smali changed")
    open(auth, "w", encoding="utf8").write(a2)
    # Android can pause the activity before the GL thread has created the surface (screen off or lock screen up while
    # the app starts); Cocos2dxRenderer then calls nativeOnPause on an engine that doesn't exist yet and crashes
    # (SIGSEGV, null pointer, Galaxy Tab S9 FE, Android 16). mLastTickInNanoSeconds is first set in onSurfaceCreated
    # right after nativeInit, so skip pause/resume while it's still 0.
    ren = os.path.join(WORK, "smali", "org", "cocos2dx", "lib", "Cocos2dxRenderer.smali")
    r = open(ren, encoding="utf8").read()
    r2 = r
    for m in ("nativeOnPause", "nativeOnResume"):
        r2 = r2.replace(
            f"    .locals 0\n\n    .prologue\n    .line {145 if m == 'nativeOnPause' else 149}\n"
            f"    invoke-static {{}}, Lorg/cocos2dx/lib/Cocos2dxRenderer;->{m}()V\n",
            f"    .locals 2\n\n    .prologue\n"
            "    iget-wide v0, p0, Lorg/cocos2dx/lib/Cocos2dxRenderer;->mLastTickInNanoSeconds:J\n"
            "    invoke-static {v0, v1}, Ljava/lang/Long;->signum(J)I\n"
            "    move-result v0\n"
            f"    if-eqz v0, :skip_{m}\n"
            f"    invoke-static {{}}, Lorg/cocos2dx/lib/Cocos2dxRenderer;->{m}()V\n"
            f"    :skip_{m}\n")
    if r2.count(":skip_") != 4:
        raise SystemExit("Cocos2dxRenderer pause/resume guard did not apply -- the smali changed")
    open(ren, "w", encoding="utf8").write(r2)
    strings = os.path.join(WORK, "res", "values", "strings.xml")
    t = open(strings, encoding="utf8").read()
    t = t.replace('<string name="app_name">Roomies!</string>', '<string name="app_name">Roomies! Offline</string>')
    open(strings, "w", encoding="utf8").write(t)


def keystore():
    ks = os.path.join(R, "offline.keystore")
    if not os.path.exists(ks):
        run(os.path.join(JDK, "keytool.exe"), "-genkeypair", "-keystore", ks, "-storepass", "roomies", "-keypass",
            "roomies", "-alias", "offline", "-keyalg", "RSA", "-keysize", "2048", "-validity", "36500",
            "-dname", "CN=Roomies Offline")
    return ks


def main():
    debug = "--debug" in sys.argv[1:]
    arm64 = "--arm64" in sys.argv[1:]
    lib64 = os.path.join(R, "native64", "build", "libotome.so")
    if arm64 and not os.path.exists(lib64):
        raise SystemExit("--arm64: build native64 first (see native64/REWRITE.md)")
    out = OUT.replace(".apk", "_debug.apk") if debug else OUT
    dex = compile_java()
    patch_tree(debug)
    base = os.path.join(B, "base.apk")
    run(os.path.join(JDK, "java.exe"), "-jar", APKTOOL, "b", "-f", "-o", base, WORK)
    full = os.path.join(B, "full.apk")
    shutil.copy(base, full)
    srv = os.path.join(B, "assets", "server")
    n = 0
    with zipfile.ZipFile(full, "a", allowZip64=True) as z:
        z.write(dex, "classes2.dex", compress_type=zipfile.ZIP_DEFLATED)
        if arm64:
            z.write(lib64, "lib/arm64-v8a/libotome.so", compress_type=zipfile.ZIP_DEFLATED)
        # Story music: the engine opens "Application/cache/" + audio.bgm from the assets (app.py HEROINE_BGM /
        # MAIN_BGM), stored uncompressed so AssetManager.openFd works. The original title BGM plus the official
        # soundtrack (offline/convert_ost.py -> static/sound/bgm/ost/*.ogg) go in the cache; each heroine's stories
        # play her first theme, the prologue the main theme.
        z.write(os.path.join(R, "decoded", "assets", "Application", "sound", "bgm", "splash", "13_01.ogg"),
                "assets/Application/cache/sound/bgm/13_01.ogg", compress_type=zipfile.ZIP_STORED)
        ost = os.path.join(R, "..", "server", "data", "static", "sound", "bgm", "ost")
        for f in sorted(os.listdir(ost)):
            if f.endswith(".ogg"):
                z.write(os.path.join(ost, f), "assets/Application/cache/sound/bgm/ost/" + f,
                        compress_type=zipfile.ZIP_STORED)
        for dp, _, fs in os.walk(srv):
            for f in fs:
                p = os.path.join(dp, f)
                arc = "assets/server/" + os.path.relpath(p, srv).replace(os.sep, "/")
                z.write(p, arc, compress_type=zipfile.ZIP_STORED if f.lower().endswith(STORE_EXT) else zipfile.ZIP_DEFLATED)
                n += 1
    aligned = os.path.join(B, "aligned.apk")
    if os.path.exists(aligned):
        os.remove(aligned)
    run(os.path.join(BT, "zipalign.exe"), "-f", "-p", "4", full, aligned)
    ks = keystore()
    if os.path.exists(out):
        os.remove(out)
    run(os.path.join(BT, "apksigner.bat"), "sign", "--ks", ks, "--ks-pass", "pass:roomies", "--key-pass", "pass:roomies",
        "--out", out, aligned, shell=False)
    print(n, "server files bundled;", out, round(os.path.getsize(out) / 2**20), "MB")


if __name__ == "__main__":
    sys.exit(main())
