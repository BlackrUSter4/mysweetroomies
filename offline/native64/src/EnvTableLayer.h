#ifndef ENVTABLELAYER_H
#define ENVTABLELAYER_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "EnvTableLayerData.h"
#include "GUI/CCScrollView/CCTableView.h"
#include "cocos2d.h"

// Scrolling two-line list used by the debug screen (environment / config values). Tapping a row passes its key and
// value to the callback. The CCTableView is child tag 0; each cell holds the key label (tag 0) and value label (tag 1).
class EnvTableLayer : public cocos2d::CCLayer,
                      public cocos2d::extension::CCTableViewDataSource,
                      public cocos2d::extension::CCTableViewDelegate {
public:
    static EnvTableLayer* create(const cocos2d::CCSize& size);
    virtual ~EnvTableLayer();
    virtual bool initWithSize(const cocos2d::CCSize& size);

    virtual void setTouchEnabled(bool enabled);  // forwarded to the table view

    void setCallback(std::function<void(const std::string&, const std::string&)> callback);
    void setData(std::map<const std::string, const std::string> data);  // replaces all rows (map order)
    void addData(const std::string& key, std::string value);
    void clearData();
    void invalidate();  // reloads the table after setData/addData/clearData

    // CCTableViewDataSource
    virtual cocos2d::CCSize tableCellSizeForIndex(cocos2d::extension::CCTableView* table, unsigned int idx);
    virtual cocos2d::extension::CCTableViewCell* tableCellAtIndex(cocos2d::extension::CCTableView* table,
                                                                  unsigned int idx);
    virtual unsigned int numberOfCellsInTableView(cocos2d::extension::CCTableView* table);

    // CCTableViewDelegate
    virtual void scrollViewDidScroll(cocos2d::extension::CCScrollView* view);
    virtual void scrollViewDidZoom(cocos2d::extension::CCScrollView* view);
    virtual void tableCellTouched(cocos2d::extension::CCTableView* table, cocos2d::extension::CCTableViewCell* cell);

private:
    std::vector<EnvTableLayerData> m_rows;                                    // orig +0x110
    std::function<void(const std::string&, const std::string&)> m_callback;  // orig +0x11c
};

#endif
