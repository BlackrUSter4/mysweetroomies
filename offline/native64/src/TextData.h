// TextData: what the text window shows for one scene (SceneData::text). Copied by value into the text log
// (TextLogManager::add).
#ifndef TEXTDATA_H
#define TEXTDATA_H

#include <string>

#include "ExEnums.h"

struct TextData {
    // Name for the name plate; "" when there is none. Never "__NULL__" (the parser replaces it).
    std::string speaker;
    // True when the JSON speaker was "__NULL__" (narration: no name plate at all, speaker is "").
    bool speakerHidden = false;
    // The line itself; "" for an empty or "__NULL__" body.
    std::string body;
    ExSceneShowTail showTail = ExSceneShowTail_None;     // window/bubble style
    ExSceneLinkScene linkScene = ExSceneLinkScene_None;  // Link: SceneManager auto-advances after MainRender

    TextData() {}
    TextData(const TextData &) = default;  // orig 001cf8c8
    TextData &operator=(const TextData &) = default;
    ~TextData() {}                         // orig 001cf8f8
};

#endif
