"""Offline APK, step 2 (2026-10-15): the server's static files for the APK (build/assets/server/static).

The 9,880 story pictures alone are 7.3 GB of PNG, too much for one APK. Every PNG under img/ is stored as WebP
(quality 90, alpha kept; about 1/8 the size and not visibly different). The Java server hands WebP straight to the
WebView, which shows it whatever the file name, and turns it back into PNG for the story engine, which only accepts
PNG. Sound, video and fonts are copied as they are. Resumable (skips files already converted).
CPU: 8 worker processes pinned to 8 cores (about 40% of a 20-thread CPU).
usage: python export_static.py"""
import os
import shutil
import sys
from multiprocessing import Pool

import cv2
import psutil

R = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(R, "..", "server", "data", "static")
DST = os.path.join(R, "build", "assets", "server", "static")
QUALITY = 90
CORES = list(range(8))


def pin():
    try:
        psutil.Process().cpu_affinity(CORES)
    except Exception:
        pass


def convert(rel):
    src = os.path.join(SRC, rel)
    out = os.path.join(DST, rel[:-4] + ".webp")
    if os.path.exists(out):
        return 0
    os.makedirs(os.path.dirname(out), exist_ok=True)
    im = cv2.imread(src, cv2.IMREAD_UNCHANGED)
    if im is None:
        return 0
    ok, buf = cv2.imencode(".webp", im, [cv2.IMWRITE_WEBP_QUALITY, QUALITY])
    with open(out + ".part", "wb") as f:
        f.write(buf.tobytes())
    os.replace(out + ".part", out)
    return 1


def main():
    pin()
    pngs, n_copy = [], 0
    for root, _, files in os.walk(SRC):
        for f in files:
            rel = os.path.relpath(os.path.join(root, f), SRC).replace("\\", "/")
            if rel.startswith("img/") and f.lower().endswith(".png"):
                pngs.append(rel)
            else:
                out = os.path.join(DST, rel)
                if not os.path.exists(out) or os.path.getsize(out) != os.path.getsize(os.path.join(SRC, rel)):
                    os.makedirs(os.path.dirname(out), exist_ok=True)
                    shutil.copy2(os.path.join(SRC, rel), out)
                    n_copy += 1
    done = 0
    with Pool(8, initializer=pin) as pool:
        for i, k in enumerate(pool.imap_unordered(convert, pngs, chunksize=16)):
            done += k
            if i % 1000 == 0:
                print(i, "/", len(pngs), flush=True)
    print(n_copy, "files copied,", done, "pictures converted,", len(pngs), "in all")


if __name__ == "__main__":
    sys.exit(main())
