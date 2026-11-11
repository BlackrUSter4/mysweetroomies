"""Download the third-party sources the arm64 build compiles and apply our one engine patch.

    python tools/fetch_deps.py

Fetches cocos2d-x 2.2.6 (the engine version the original libotome.so was built from) and libpng 1.6.44 next to
CMakeLists.txt, then compiles JPEG/TIFF out of CCImage: the game only ever loads PNGs, and the 2014 prebuilt
libjpeg/libtiff/libwebp packages have no arm64 build. Safe to re-run; existing trees are kept.
"""
import io
import pathlib
import tarfile
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
COCOS = ROOT / "cocos2d-x-cocos2d-x-2.2.6"
PNG = ROOT / "libpng-1.6.44"


def fetch(url):
    print("downloading", url)
    with urllib.request.urlopen(url) as r:
        return r.read()


def patch_ccimage():
    path = COCOS / "cocos2dx" / "platform" / "CCImageCommon_cpp.h"
    lines = path.read_bytes().decode("utf-8").split("\n")
    if any("CC_USE_JPEG_TIFF" in l for l in lines):
        return
    # 1-based (first, last) line of each block in the 2.2.6 file, checked before patching
    blocks = [
        (255, 400, "", "bool CCImage::_initWithJpgData(void * data, int nSize) "
                         "{ CC_UNUSED_PARAM(data); CC_UNUSED_PARAM(nSize); return false; }"),
        (527, 703, "", "bool CCImage::_initWithTiffData(void* pData, int nDataLen) "
                       "{ CC_UNUSED_PARAM(pData); CC_UNUSED_PARAM(nDataLen); return false; }"),
        (907, 983, "bool CCImage::_saveImageToJPG", "bool CCImage::_saveImageToJPG(const char * pszFilePath) "
                                                    "{ CC_UNUSED_PARAM(pszFilePath); return false; }"),
    ]
    assert lines[34].startswith('#include "jpeglib.h"') and lines[35].startswith('#include "tiffio.h"'), \
        "unexpected CCImageCommon_cpp.h -- not cocos2d-x 2.2.6?"
    for first, last, starts, _ in blocks:
        assert lines[first - 1].strip().startswith(starts) and lines[last - 1].rstrip("\r") == "}", (first, last)
    for first, last, _, stub in reversed(blocks):
        lines.insert(last, "#else\n" + stub + "\n#endif")
        lines.insert(first - 1, "#if CC_USE_JPEG_TIFF")
    lines[34] = '#if CC_USE_JPEG_TIFF\n#include "jpeglib.h"\n#include "tiffio.h"\n#endif'
    lines[35] = ""
    path.write_bytes("\n".join(lines).encode("utf-8"))
    print("patched", path.relative_to(ROOT))


def main():
    if not COCOS.exists():
        data = fetch("https://github.com/cocos2d/cocos2d-x/archive/refs/tags/cocos2d-x-2.2.6.zip")
        zipfile.ZipFile(io.BytesIO(data)).extractall(ROOT)
    patch_ccimage()
    if not PNG.exists():
        data = fetch("https://download.sourceforge.net/libpng/libpng-1.6.44.tar.gz")
        tarfile.open(fileobj=io.BytesIO(data)).extractall(ROOT, filter="data")
    print("ok")


if __name__ == "__main__":
    main()
