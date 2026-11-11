// TextLogDialog: modal backlog window (opened from OverlayMenuLayer). A CCTableView (kCCTableViewFillBottomUp) of
// every line in TextLogManager, each cell a name label (tag 0) over a body label (tag 1), plus a close
// button at the top right of the frame. Taps outside do not close it.
#ifndef TEXTLOGDIALOG_H
#define TEXTLOGDIALOG_H

#include <vector>

#include "MiModalLayer.h"
#include "TextData.h"
#include "GUI/CCScrollView/CCTableView.h"
#include "cocos2d.h"

class TextLogDialog : public MiModalLayer,
                      public cocos2d::extension::CCTableViewDataSource,
                      public cocos2d::extension::CCTableViewDelegate {
public:
    static TextLogDialog *create();
    static cocos2d::CCScene *scene();  // a scene holding only the dialog
    virtual ~TextLogDialog();
    virtual bool init();     // MiModalLayer::init, snapshot of TextLogManager::getAll(), initUi()
    virtual void initUi();

    void onCloseButtonClicked(cocos2d::CCObject *sender);  // dismiss()

    // CCScrollViewDelegate / CCTableViewDelegate: nothing to do.
    virtual void scrollViewDidScroll(cocos2d::extension::CCScrollView *view);
    virtual void scrollViewDidZoom(cocos2d::extension::CCScrollView *view);
    virtual void tableCellTouched(cocos2d::extension::CCTableView *table,
                                  cocos2d::extension::CCTableViewCell *cell);

    // CCTableViewDataSource
    virtual cocos2d::CCSize tableCellSizeForIndex(cocos2d::extension::CCTableView *table, unsigned int idx);
    virtual cocos2d::extension::CCTableViewCell *tableCellAtIndex(cocos2d::extension::CCTableView *table,
                                                                  unsigned int idx);
    virtual unsigned int numberOfCellsInTableView(cocos2d::extension::CCTableView *table);

private:
    TextLogDialog();

    std::vector<TextData> m_logs;  // taken once in init(); the dialog does not follow later additions
};

#endif
