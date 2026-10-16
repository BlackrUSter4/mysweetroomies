"""Offline APK, step 3 (2026-10-15): build "My Sweet Roomies! (Offline)" with the mock server inside.

Starting from the decoded original (offline/decoded, apktool 2.10.0):
  1. app.json (assets/Application/document/json/*/android) points site_url / resource_url at http://127.0.0.1:18765
  2. the manifest gets package jp.mfapps.novel.roomies.offline (so it installs next to the original), its own
     C2D_MESSAGE permission name, and a content provider mock.offline.Boot that starts the server at launch
  3. apktool rebuilds it; then the Java server (offline/java, compiled to classes2.dex) and assets/server
     (export_data.py, export_static.py) are added, pictures/sound/video stored uncompressed
  4. zipalign, then apksigner with offline/offline.keystore (made on first run)
The server's AES key comes from the APK's own signature at run time, so any signing key works.
usage: python build_apk.py        (run export_data.py and export_static.py first)"""
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


def patch_tree():
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
    s = s.replace("jp.mfapps.novel.roomies.permission.C2D_MESSAGE", PKG + ".permission.C2D_MESSAGE")
    provider = (f'<provider android:name="mock.offline.Boot" android:authorities="{PKG}.mockserver" '
                'android:exported="false" android:initOrder="1000"/>')
    s = s.replace("</application>", provider + "</application>", 1)
    open(man, "w", encoding="utf8").write(s)
    yml = os.path.join(WORK, "apktool.yml")
    y = open(yml, encoding="utf8").read().replace("renameManifestPackage: null", f"renameManifestPackage: {PKG}")
    open(yml, "w", encoding="utf8").write(y)
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
    dex = compile_java()
    patch_tree()
    base = os.path.join(B, "base.apk")
    run(os.path.join(JDK, "java.exe"), "-jar", APKTOOL, "b", "-f", "-o", base, WORK)
    full = os.path.join(B, "full.apk")
    shutil.copy(base, full)
    srv = os.path.join(B, "assets", "server")
    n = 0
    with zipfile.ZipFile(full, "a", allowZip64=True) as z:
        z.write(dex, "classes2.dex", compress_type=zipfile.ZIP_DEFLATED)
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
    if os.path.exists(OUT):
        os.remove(OUT)
    run(os.path.join(BT, "apksigner.bat"), "sign", "--ks", ks, "--ks-pass", "pass:roomies", "--key-pass", "pass:roomies",
        "--out", OUT, aligned, shell=False)
    print(n, "server files bundled;", OUT, round(os.path.getsize(OUT) / 2**20), "MB")


if __name__ == "__main__":
    sys.exit(main())
