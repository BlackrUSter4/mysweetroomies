# arm64 rebuild of libotome.so: stock cocos2d-x 2.2.6 + libpng + the game's own code (src/).
# Configure: cmake -G Ninja -B build -DCMAKE_TOOLCHAIN_FILE=<ndk>/build/cmake/android.toolchain.cmake
#            -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 3.18)
project(otome C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(COCOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d-x-cocos2d-x-2.2.6)
set(PNG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/libpng-1.6.44)
include(cmake/sources.cmake)

# cocos2d-x 2.2.6 is C++03 code full of narrowing conversions and deprecated idioms; keep the noise down.
set(LEGACY_FLAGS -Wno-everything -fno-strict-aliasing -fsigned-char)
set(CMAKE_CXX_STANDARD 11)

# --- libpng (no NEON: the hand-written ARM paths are optional and need extra sources) ---
configure_file(${PNG_ROOT}/scripts/pnglibconf.h.prebuilt ${CMAKE_BINARY_DIR}/png/pnglibconf.h COPYONLY)
add_library(png STATIC
  ${PNG_ROOT}/png.c ${PNG_ROOT}/pngerror.c ${PNG_ROOT}/pngget.c ${PNG_ROOT}/pngmem.c ${PNG_ROOT}/pngpread.c
  ${PNG_ROOT}/pngread.c ${PNG_ROOT}/pngrio.c ${PNG_ROOT}/pngrtran.c ${PNG_ROOT}/pngrutil.c ${PNG_ROOT}/pngset.c
  ${PNG_ROOT}/pngtrans.c ${PNG_ROOT}/pngwio.c ${PNG_ROOT}/pngwrite.c ${PNG_ROOT}/pngwtran.c ${PNG_ROOT}/pngwutil.c)
target_include_directories(png PUBLIC ${PNG_ROOT} ${CMAKE_BINARY_DIR}/png)
target_compile_definitions(png PRIVATE PNG_ARM_NEON_OPT=0)
target_compile_options(png PRIVATE -Wno-everything)

# --- cocos2dx core ---
list(FILTER COCOS2DX_SRCS EXCLUDE REGEX "CCImageCommonWebp\.cpp$")
add_library(cocos2dx STATIC ${COCOS2DX_SRCS} stubs/CCImageWebpStub.cpp)
target_include_directories(cocos2dx PUBLIC
  ${COCOS_ROOT}/cocos2dx ${COCOS_ROOT}/cocos2dx/include ${COCOS_ROOT}/cocos2dx/kazmath/include
  ${COCOS_ROOT}/cocos2dx/platform/android)
target_compile_definitions(cocos2dx PUBLIC USE_FILE32API ANDROID CC_USE_JPEG_TIFF=0)
target_compile_options(cocos2dx PRIVATE ${LEGACY_FLAGS})
target_link_libraries(cocos2dx PUBLIC png GLESv2 EGL log z android)

# --- the game library the Java side loads (System.loadLibrary("otome")) ---
file(GLOB_RECURSE GAME_SRCS CONFIGURE_DEPENDS src/*.cpp)
add_library(otome SHARED ${GAME_SRCS})
target_include_directories(otome PRIVATE src)
# Whole-archive: the engine's JNI entry points (Cocos2dxRenderer.nativeRender, touches, Cocos2dxHelper...) are only
# called from Java, so a normal static link would drop them (ndk-build used LOCAL_WHOLE_STATIC_LIBRARIES for this).
target_link_libraries(otome PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,cocos2dx>")
target_link_options(otome PRIVATE -Wl,--gc-sections -Wl,-z,max-page-size=16384)

# --- CocosDenshion (CCBReader plays sounds through it) ---
add_library(cocos_denshion STATIC ${COCOS_ROOT}/CocosDenshion/android/SimpleAudioEngine.cpp
  ${COCOS_ROOT}/CocosDenshion/android/jni/SimpleAudioEngineJni.cpp)
target_include_directories(cocos_denshion PUBLIC ${COCOS_ROOT}/CocosDenshion/include)
target_compile_options(cocos_denshion PRIVATE ${LEGACY_FLAGS})
target_link_libraries(cocos_denshion PUBLIC cocos2dx)

# --- the three extension modules the game uses: CocosBuilder reader (.ccbi), CCControl (sliders), CCScrollView/CCTableView ---
list(FILTER EXTENSIONS_SRCS INCLUDE REGEX "/(CCBReader|GUI/CCControlExtension|GUI/CCScrollView)/")
add_library(cocos_ext STATIC ${EXTENSIONS_SRCS})
target_include_directories(cocos_ext PUBLIC ${COCOS_ROOT}/extensions ${COCOS_ROOT})
target_compile_options(cocos_ext PRIVATE ${LEGACY_FLAGS})
target_link_libraries(cocos_ext PUBLIC cocos2dx cocos_denshion)
target_link_libraries(otome PRIVATE cocos_ext)
target_include_directories(otome PRIVATE third_party ${COCOS_ROOT}/extensions/CocoStudio/Json)
