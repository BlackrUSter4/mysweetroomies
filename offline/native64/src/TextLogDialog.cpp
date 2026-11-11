#include "TextLogDialog.h"

#include "TextLogManager.h"

USING_NS_CC;
USING_NS_CC_EXT;

namespace {

const char *const ATLAS_PLIST = "Framework/ui/menu_textlog.plist";
const char *const ATLAS_TEXTURE = "Framework/ui/menu_textlog.png";
const char *const CELL_FONT = "arial";
const ccColor3B NAME_COLOR = {44, 44, 44};
const ccColor3B BODY_COLOR = {44, 44, 44};
const int NAME_LABEL_TAG = 0;
const int BODY_LABEL_TAG = 1;

// Width of the 640-pixel design space on this screen.
float designScale()
{
    return CCDirector::sharedDirector()->getWinSize().width / 640.0f;
}

}  // namespace

TextLogDialog::TextLogDialog() {}

// orig 001e1214
TextLogDialog *TextLogDialog::create()
{
    TextLogDialog *dialog = new TextLogDialog();
    if (!dialog->init()) {
        delete dialog;
        return nullptr;
    }
    dialog->autorelease();
    return dialog;
}

// orig 001e2250
CCScene *TextLogDialog::scene()
{
    CCScene *scene = CCScene::create();
    scene->addChild(create());
    return scene;
}

// orig 001e2298
TextLogDialog::~TextLogDialog() {}

// orig 001e239c
bool TextLogDialog::init()
{
    if (!MiModalLayer::init()) return false;
    m_logs = TextLogManager::getInstance()->getAll();
    initUi();
    return true;
}

// orig 001e1d0c
void TextLogDialog::initUi()
{
    CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(ATLAS_PLIST, ATLAS_TEXTURE);
    float scale = winSize.width / 640.0f;
    float margin = scale * 30.0f;

    CCSprite *frame = CCSprite::createWithSpriteFrameName("bg_log.png");
    frame->setScale(scale);
    frame->setAnchorPoint(CCPoint(0.5f, 0.0f));
    frame->setPosition(CCPoint(winSize.width * 0.5f, margin));
    addChild(frame);

    CCTableView *table = CCTableView::create(this, CCSize(scale * 640.0f, scale * 750.0f));
    table->ignoreAnchorPointForPosition(false);
    table->setAnchorPoint(CCPoint(0.5f, 0.0f));
    table->setPosition(CCPoint(winSize.width * 0.5f, margin + margin));
    table->setVerticalFillOrder(kCCTableViewFillBottomUp);
    table->setDelegate(this);
    // Above the modal layer's own touch swallowing, below the close button.
    table->setTouchPriority(MiModalLayer::kModalLayerPriority - 1);
    addChild(table);

    CCMenuItemSprite *closeItem = CCMenuItemSprite::create(
        CCSprite::createWithSpriteFrameName("btn_window_close.png"),
        CCSprite::createWithSpriteFrameName("btn_window_close_active.png"), this,
        menu_selector(TextLogDialog::onCloseButtonClicked));
    closeItem->setScale(scale);
    // Anchor past the item's own corner pulls the button inwards from the frame's top right corner.
    closeItem->setAnchorPoint(CCPoint(1.3f, 1.3f));
    CCMenu *menu = CCMenu::create(closeItem, NULL);
    CCRect frameBox = frame->boundingBox();
    menu->setPosition(CCPoint(frameBox.getMaxX(), frameBox.getMaxY()));
    menu->setTouchPriority(MiModalLayer::kModalLayerPriority - 2);
    addChild(menu);

    setEnableCloseOnTouchLayer(false);
}

// orig 001e1ca8
void TextLogDialog::onCloseButtonClicked(CCObject *)
{
    dismiss();
}

// orig 001e1c90
void TextLogDialog::scrollViewDidScroll(CCScrollView *) {}

// orig 001e1c30
void TextLogDialog::scrollViewDidZoom(CCScrollView *) {}

// orig 001e1c78
void TextLogDialog::tableCellTouched(CCTableView *, CCTableViewCell *) {}

// orig 001e1cb8
CCSize TextLogDialog::tableCellSizeForIndex(CCTableView *, unsigned int)
{
    float scale = designScale();
    return CCSize(scale * 640.0f, scale * 260.0f);
}

// orig 001e1f34
CCTableViewCell *TextLogDialog::tableCellAtIndex(CCTableView *table, unsigned int idx)
{
    // at() like the original: an index past the snapshot throws std::out_of_range.
    const TextData &line = m_logs.at(idx);

    CCTableViewCell *cell = table->dequeueCell();
    if (cell) {
        static_cast<CCLabelTTF *>(cell->getChildByTag(NAME_LABEL_TAG))->setString(line.speaker.c_str());
        static_cast<CCLabelTTF *>(cell->getChildByTag(BODY_LABEL_TAG))->setString(line.body.c_str());
        return cell;
    }

    cell = new CCTableViewCell();
    cell->autorelease();
    float scale = designScale();
    cell->setContentSize(CCSize(scale * 640.0f, scale * 260.0f));

    CCSprite *box = CCSprite::createWithSpriteFrameName("log_box_bg.png");
    box->setPosition(CCPoint(cell->getContentSize().width * 0.5f, cell->getContentSize().height * 0.5f));
    box->setScale(scale);
    cell->addChild(box);

    float fontSize = scale * 32.0f;
    float textLeft = scale * 50.0f;
    float textWidth = scale * 540.0f;

    CCLabelTTF *name = CCLabelTTF::create(line.speaker.c_str(), CELL_FONT, fontSize);
    name->setAnchorPoint(CCPoint(0.0f, 0.5f));
    name->setPosition(CCPoint(textLeft, scale * 208.0f));
    name->setContentSize(CCSize(textWidth, scale * 67.0f));
    name->setDimensions(name->getContentSize());
    name->setTag(NAME_LABEL_TAG);
    name->setHorizontalAlignment(kCCTextAlignmentLeft);
    name->setColor(NAME_COLOR);
    cell->addChild(name);

    CCLabelTTF *body = CCLabelTTF::create(line.body.c_str(), CELL_FONT, fontSize);
    body->setAnchorPoint(CCPoint(0.0f, 1.0f));
    body->setPosition(CCPoint(textLeft, scale * 178.0f));
    body->setContentSize(CCSize(textWidth, scale * 176.0f));
    body->setDimensions(body->getContentSize());
    body->setTag(BODY_LABEL_TAG);
    body->setHorizontalAlignment(kCCTextAlignmentLeft);
    body->setVerticalAlignment(kCCVerticalTextAlignmentTop);
    body->setColor(BODY_COLOR);
    cell->addChild(body);
    return cell;
}

// orig 001e1c48
unsigned int TextLogDialog::numberOfCellsInTableView(CCTableView *)
{
    return static_cast<unsigned int>(m_logs.size());
}
