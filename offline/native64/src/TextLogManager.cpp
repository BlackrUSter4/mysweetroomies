#include "TextLogManager.h"

#include <cstring>

#include "ResourceManager.h"
#include "SceneManager.h"
#include "cocos2d.h"

USING_NS_CC;

namespace {

const char *const LISTENER_NAME = "text_log_manager";
const char *const LOG_FILE_NAME = "textlog.json";  // under CCFileUtils::getWritablePath()
const char *const KEY_NAME = "name";
const char *const KEY_BODY = "body";

void reportDone(SceneEventType event)
{
    SceneManager::getInstance()->doneListenerEvent(event, LISTENER_NAME);
}

std::string stringMember(const picojson::object &entry, const char *key)
{
    picojson::object::const_iterator found = entry.find(key);
    // The original read the string out of the value without a type check; anything else counts as absent here.
    if (found == entry.end() || !found->second.is<std::string>()) return "";
    return found->second.get<std::string>();
}

}  // namespace

const int TextLogManager::MAX_LOG_SIZE = 80;

TextLogManager::TextLogManager()
{
    init();
}

// orig 001c904c
TextLogManager::~TextLogManager() {}

// orig 001c9078
TextLogManager *TextLogManager::getInstance()
{
    static TextLogManager instance;
    return &instance;
}

// orig 001d0250
bool TextLogManager::init()
{
    setMaxLogSize(MAX_LOG_SIZE);
    setDefaultIO();
    return true;
}

// orig 001d1538
void TextLogManager::prepare(LoadFunction load, SaveFunction save)
{
    initListener();
    setIO(load, save);
    this->load();
}

// orig 001cfcf4
void TextLogManager::initListener()
{
    auto listener = std::make_shared<SceneEventListener>(
        [](const std::vector<std::shared_ptr<SceneData>> &) { reportDone(SceneEventType_Setup); },  // orig 001cfbfc
        [](std::shared_ptr<SceneData>) { reportDone(SceneEventType_MainRender); },                // orig 001cfc14
        // A line enters the log as soon as its scene starts rendering (orig 001d1020).
        [](std::shared_ptr<SceneData>, std::shared_ptr<SceneData> current) {
            if (current && current->text) {
                TextLogManager::getInstance()->add(*current->text)->save();
            }
            reportDone(SceneEventType_PreRender);
        },
        [](std::shared_ptr<SceneData>, std::shared_ptr<SceneData>) { reportDone(SceneEventType_PreEffect); },  // orig 001cfc4c
        [](std::shared_ptr<SceneData>) { reportDone(SceneEventType_PostEffect); },  // orig 001cfca4
        []() { reportDone(SceneEventType_Finish); });                                // orig 001cfcdc
    // Replaced rather than kept: addListener ignores a name that is already registered.
    SceneManager::getInstance()->removeListener(LISTENER_NAME);
    SceneManager::getInstance()->addListener(LISTENER_NAME, listener);
}

// orig 001d0140
void TextLogManager::setIO(LoadFunction load, SaveFunction save)
{
    m_save = save;
    m_load = load;
}

// orig 001d01b8
void TextLogManager::setDefaultIO()
{
    // orig 001cf830
    LoadFunction load = []() -> const std::string {
        std::string path = CCFileUtils::sharedFileUtils()->getWritablePath() + LOG_FILE_NAME;
        ResourceManager *resources = ResourceManager::getInstance();
        if (!resources->isExistFile(path)) return "[]";
        return resources->slurp(path);
    };
    // orig 001cf6b4: the original builds the same path and then writes nothing, so the default log is read-only.
    SaveFunction save = [](const std::string &) {};
    setIO(load, save);
}

// orig 001d0fb8
TextLogManager *TextLogManager::add(TextData text)
{
    m_logs.push_back(deflate(text));
    // Compared unsigned like the original, so a negative maximum keeps everything.
    while (m_logs.size() > static_cast<unsigned int>(getMaxLogSize())) removeFirst();
    return this;
}

// orig 001d0a72
bool TextLogManager::removeFirst()
{
    if (m_logs.empty()) return false;  // the original erased from an empty vector here (undefined behaviour)
    m_logs.erase(m_logs.begin());
    return true;
}

// orig 001d039a
void TextLogManager::clear()
{
    m_logs.clear();
}

// orig 001d0b80
void TextLogManager::save()
{
    std::string json = picojson::value(m_logs).serialize();
    if (m_save) m_save(json);  // the original called the std::function unchecked (throws bad_function_call)
}

// orig 001d14cc
void TextLogManager::load()
{
    if (!m_load) return;  // see save()
    m_logs = parse(m_load());
}

// orig 001d0e54
std::vector<TextData> TextLogManager::getAll()
{
    std::vector<TextData> all;
    all.reserve(m_logs.size());
    for (const picojson::value &entry : m_logs) all.push_back(inflate(entry));
    return all;
}

// orig 001cf90c
int TextLogManager::getSize()
{
    return static_cast<int>(m_logs.size());
}

// orig 001cf916
void TextLogManager::setMaxLogSize(int size)
{
    m_maxLogSize = size;
}

// orig 001cf91a
int TextLogManager::getMaxLogSize()
{
    return m_maxLogSize;
}

// orig 001d0ed8
picojson::value TextLogManager::deflate(const TextData &text)
{
    picojson::object entry;
    entry[KEY_NAME] = picojson::value(text.speaker);
    entry[KEY_BODY] = picojson::value(text.body);
    return picojson::value(entry);
}

// orig 001d0d34
TextData TextLogManager::inflate(const picojson::value &entry)
{
    TextData text;
    // A non-object entry (only possible with a hand-edited log) gives an empty line instead of the original's
    // blind read of the union.
    if (!entry.is<picojson::object>()) return text;
    const picojson::object &object = entry.get<picojson::object>();
    text.speaker = stringMember(object, KEY_NAME);
    text.body = stringMember(object, KEY_BODY);
    return text;
}

// orig 001d1400
std::vector<picojson::value> TextLogManager::parse(const std::string &json)
{
    picojson::value root;
    const char *begin = json.c_str();
    std::string error;
    picojson::parse(root, begin, begin + std::strlen(begin), &error);
    // The original ignored the error and copied the array out of whatever it got (null after a syntax error).
    if (!error.empty() || !root.is<picojson::array>()) return std::vector<picojson::value>();
    return root.get<picojson::array>();
}
