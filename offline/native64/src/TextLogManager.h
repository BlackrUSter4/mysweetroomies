// TextLogManager: the backlog - the last MAX_LOG_SIZE lines the player has seen, shown by TextLogDialog.
//
// Lines are kept as picojson objects {"name": speaker, "body": body} and saved as one JSON array through the save
// function after every new line; load() replaces the log with whatever the load function returns.
//
//   TextLogManager::getInstance()->prepare(loadFn, saveFn);   // ConversationScene, before the story starts
//
// prepare() registers SceneManager listener "text_log_manager" (every PreRender appends the current scene's text,
// then saves), installs the IO functions and loads. The default IO (setDefaultIO, used until prepare) loads
// <writable path>/textlog.json ("[]" when the file is missing) and saves nothing.
#ifndef TEXTLOGMANAGER_H
#define TEXTLOGMANAGER_H

#include <functional>
#include <string>
#include <vector>

#include "TextData.h"
#include "picojson.h"

class TextLogManager {
public:
    typedef std::function<const std::string()> LoadFunction;        // -> JSON array text
    typedef std::function<void(const std::string &)> SaveFunction;  // (JSON array text)

    static const int MAX_LOG_SIZE;  // 80: default log length

    static TextLogManager *getInstance();  // process-wide instance, created (and init()ed) on first use
    virtual ~TextLogManager();
    bool init();  // max size MAX_LOG_SIZE, default IO

    void prepare(LoadFunction load, SaveFunction save);  // initListener + setIO + load
    void initListener();
    void setIO(LoadFunction load, SaveFunction save);
    void setDefaultIO();

    // Appends a line (only speaker and body are kept), dropping the oldest ones beyond getMaxLogSize(). Returns this.
    TextLogManager *add(TextData text);
    bool removeFirst();
    void clear();
    void save();
    void load();
    std::vector<TextData> getAll();  // oldest first; showTail/linkScene are the TextData defaults
    int getSize();
    void setMaxLogSize(int size);
    int getMaxLogSize();

    picojson::value deflate(const TextData &text);
    TextData inflate(const picojson::value &entry);
    // Elements of a JSON array; empty for invalid JSON or a non-array.
    std::vector<picojson::value> parse(const std::string &json);

private:
    TextLogManager();

    SaveFunction m_save;
    LoadFunction m_load;
    std::vector<picojson::value> m_logs;
    int m_maxLogSize = MAX_LOG_SIZE;
};

#endif
