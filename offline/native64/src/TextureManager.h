#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <map>
#include <string>

#include "cocos2d.h"

// Registry of sprite-frame atlases with a per-atlas display scale.
//
//   TextureManager::addAtlas("menu", "Framework/ui/conversation.plist", "Framework/ui/conversation.png", 0.5f);
//   cocos2d::CCSprite* s = TextureManager::getScaledSprite("menu", "button.png");  // autoreleased, scaled 0.5
class TextureManager {
public:
    static TextureManager* getInstance();
    virtual ~TextureManager();
    bool init();

    // Loads `plist` into CCSpriteFrameCache and remembers `scale` for `atlasName` (overwriting a previous one).
    // `texture` is accepted but unused, as in the original (the plist names its texture).
    static void addAtlas(const std::string& atlasName, const std::string& plist, const std::string& texture,
                         float scale);
    // CCSprite::createWithSpriteFrameName(frameName) scaled by the atlas' scale. Throws std::out_of_range
    // ("map::at") when `atlasName` was never added.
    static cocos2d::CCSprite* getScaledSprite(const std::string& atlasName, const std::string& frameName);

private:
    TextureManager() {}
    TextureManager& _addAtlas(const std::string& atlasName, const std::string& plist, const std::string& texture,
                              float scale);
    cocos2d::CCSprite* _getScaledSprite(const std::string& atlasName, const std::string& frameName);

    std::map<std::string, float> m_atlasScales;
};

#endif
