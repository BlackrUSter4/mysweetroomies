"""Turns the LOCAL_SRC_FILES lists of cocos2d-x 2.2.6's Android.mk files into CMake lists, so the
CMake build compiles exactly what ndk-build would (ndk-build can't cope with spaces in our path)."""
import re, pathlib
root = pathlib.Path(__file__).resolve().parent.parent / "cocos2d-x-cocos2d-x-2.2.6"
def srcs(mk):
    text = (root / mk).read_text().replace("\r\n", "\n").replace("\\\n", " ")
    m = re.search(r"LOCAL_SRC_FILES\s*:=([^\n]*)", text)
    return m.group(1).split()
out = []
for var, mk in [("COCOS2DX_SRCS", "cocos2dx/Android.mk"), ("EXTENSIONS_SRCS", "extensions/Android.mk"),
                ("BOX2D_SRCS", "external/Box2D/Android.mk")]:
    base = (root / mk).parent.relative_to(root).as_posix()
    files = [f for f in srcs(mk)]
    out.append("set(%s\n%s)\n" % (var, "\n".join("  ${COCOS_ROOT}/%s/%s" % (base, f) for f in files)))
(pathlib.Path(__file__).parent / "sources.cmake").write_text("\n".join(out))
print({v: len(srcs(m)) for v, m in [("cocos", "cocos2dx/Android.mk"), ("ext", "extensions/Android.mk"), ("box2d", "external/Box2D/Android.mk")]})
