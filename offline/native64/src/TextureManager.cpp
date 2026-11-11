#include "TextureManager.h"

using namespace cocos2d;

// orig 001d15f0
TextureManager* TextureManager::getInstance() {
    static TextureManager instance;
    static bool initialized = instance.init();
    (void)initialized;
    return &instance;
}

// orig 001d15d4
TextureManager::~TextureManager() {}

// orig 001d15a8
bool TextureManager::init() {
    return true;
}

// orig 001d18a8
TextureManager& TextureManager::_addAtlas(const std::string& atlasName, const std::string& plist,
                                          const std::string& texture, float scale) {
    (void)texture;
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(plist.c_str());
    m_atlasScales[atlasName] = scale;
    return *this;
}

// orig 001d1924
void TextureManager::addAtlas(const std::string& atlasName, const std::string& plist, const std::string& texture,
                              float scale) {
    getInstance()->_addAtlas(atlasName, plist, texture, scale);
}

// orig 001d1678
CCSprite* TextureManager::_getScaledSprite(const std::string& atlasName, const std::string& frameName) {
    CCSprite* sprite = CCSprite::createWithSpriteFrameName(frameName.c_str());
    sprite->setScale(m_atlasScales.at(atlasName));
    return sprite;
}

// orig 001d16dc
CCSprite* TextureManager::getScaledSprite(const std::string& atlasName, const std::string& frameName) {
    return getInstance()->_getScaledSprite(atlasName, frameName);
}
