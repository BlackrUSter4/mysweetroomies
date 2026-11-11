"""Every Java_* function the original libotome.so exports must exist in the rewrite, or Java dies with
UnsatisfiedLinkError the first time it calls that native method.   python tools/check_exports.py"""
import os, pathlib, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parent.parent
NM = pathlib.Path(os.environ["LOCALAPPDATA"]) / "Android/Sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-nm.exe"
def jni(so):
    out = subprocess.run([str(NM), "-D", "--defined-only", str(so)], capture_output=True, text=True, check=True).stdout
    return {l.split()[-1] for l in out.splitlines() if l.split() and l.split()[-1].startswith(("Java_", "JNI_OnLoad"))}
orig, new = jni(ROOT.parent / "decoded/lib/armeabi/libotome.so"), jni(ROOT / "build/libotome.so")
missing = sorted(orig - new)
print("%d/%d original JNI exports present" % (len(orig) - len(missing), len(orig)))
for m in missing: print("  missing", m)
for e in sorted(new - orig): print("  extra  ", e)
sys.exit(1 if missing else 0)
