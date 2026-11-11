"""Syntax-check game sources with the exact flags the real build uses, without touching the shared build directory.

    python tools/check.py src/SceneManager.cpp src/ConversationScene.cpp
    python tools/check.py            # every src/**/*.cpp

Several people (or agents) can run this at once: it only runs `clang++ -fsyntax-only`, writes nothing, and doesn't
need the other subsystems' .cpp files to exist yet, only their headers.
"""
import json
import pathlib
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = pathlib.Path(__file__).resolve().parent.parent
cc = json.loads((ROOT / "build" / "compile_commands.json").read_text())
template = next(e for e in cc if e["file"].replace("\\", "/").endswith("src/main.cpp"))
# clang on Windows accepts forward slashes everywhere, which lets shlex keep quoted -I"A:/Private Servers/..." intact
args = shlex.split(template["command"].replace(chr(92), "/"))
# drop the output/dep-file and source arguments; keep compiler + flags
flags, skip = [], False
for a in args:
    if skip:
        skip = False
        continue
    if a in ("-o", "-MF", "-MT", "-c"):
        skip = True
        continue
    if a in ("-MD",) or a.replace("\\", "/").endswith("src/main.cpp"):
        continue
    flags.append(a)
flags += ["-fsyntax-only", "-Wall", "-Wno-unused-private-field", "-Wno-overloaded-virtual", "-Wno-unused-variable",
          "-Wno-reorder-ctor", "-Wno-unused-function", "-Wno-deprecated-declarations", "-Wno-inconsistent-missing-override",
          "-ferror-limit=40"]

files = [pathlib.Path(f).resolve() for f in sys.argv[1:]] or sorted((ROOT / "src").rglob("*.cpp"))


def check(f):
    r = subprocess.run(flags + [str(f)], capture_output=True, text=True, cwd=template["directory"])
    return f, r.returncode, r.stderr


failed = 0
with ThreadPoolExecutor(max_workers=3) as pool:
    for f, code, err in pool.map(check, files):
        rel = f.relative_to(ROOT) if f.is_relative_to(ROOT) else f
        if code:
            failed += 1
            print("FAIL", rel)
            print(err.strip())
        else:
            print("ok  ", rel, "(%d warnings)" % err.count("warning:") if err.count("warning:") else "")
print("%d/%d files failed" % (failed, len(files)))
sys.exit(1 if failed else 0)
