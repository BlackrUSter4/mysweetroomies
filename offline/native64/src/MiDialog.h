#ifndef MIDIALOG_H
#define MIDIALOG_H

#include "MiModalLayer.h"
#include "cocos2d.h"

// Modal dialog base: MiModalLayer + a stored dialog size and an initUi() hook subclasses override.
class MiDialog : public MiModalLayer {
public:
    // GUESS: zero-initialised in the original (.bss, no initialiser writes it, nothing reads it).
    static int kDialogPriority;

    virtual ~MiDialog();

    virtual bool init();                                  // MiModalLayer::init, then initUi()
    virtual void initUi();                                // empty here
    virtual void onCloseClicked(cocos2d::CCObject* sender);  // dismiss()

    void setDialogSize(cocos2d::CCSize size);
    cocos2d::CCSize getDialogSize();

protected:
    cocos2d::CCSize m_dialogSize;  // orig +0x11c
};

#endif
