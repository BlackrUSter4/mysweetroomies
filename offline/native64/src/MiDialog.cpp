#include "MiDialog.h"

USING_NS_CC;

int MiDialog::kDialogPriority = 0;

MiDialog::~MiDialog() {}

// orig 001dc4e4
bool MiDialog::init() {
    if (!MiModalLayer::init()) return false;
    initUi();
    return true;
}

void MiDialog::initUi() {}

void MiDialog::onCloseClicked(CCObject*) {
    dismiss();
}

void MiDialog::setDialogSize(CCSize size) {
    m_dialogSize = size;
}

CCSize MiDialog::getDialogSize() {
    return m_dialogSize;
}
