#include "CsdUIWidgets.h"

#include "CsdUILayout.h"

#include <algorithm>

#include <cocos/ui/CocosGUI.h>

using namespace cocos2d;
using namespace cocos2d::ui;

namespace Csd {

    namespace {

        Button *createIconButton(const char *normal, const char *pressed,
                                 const Size &size, const Vec2 &pos,
                                 const Vec2 &anchor, const char *iconPath,
                                 const Size &iconSize,
                                 const Color3B &iconColor = Color3B::WHITE) {
            auto *btn = Button::create(normal, pressed, normal);
            btn->setScale9Enabled(true);
            setWidgetSize(btn, size);
            btn->setAnchorPoint(anchor);
            btn->setPosition(pos);
            if(iconPath) {
                auto *icon = ImageView::create(iconPath);
                setWidgetSize(icon, iconSize);
                icon->setAnchorPoint(Vec2(0.5f, 0.5f));
                icon->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
                icon->setColor(iconColor);
                btn->addChild(icon);
            }
            return btn;
        }

        Layout *createHighlightPanel(const LayoutCtx &ctx) {
            auto *panel = Layout::create();
            panel->setName("highlight");
            panel->setContentSize(ctx.size);
            panel->setAnchorPoint(Vec2::ZERO);
            panel->setPosition(Vec2::ZERO);
            panel->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            panel->setBackGroundColor(Color3B(192, 192, 192));
            panel->setBackGroundColorOpacity(102);
            panel->setVisible(false);
            return panel;
        }

        Layout *createRowSeparator(const LayoutCtx &ctx, float width) {
            auto *sep = Layout::create();
            sep->setContentSize(Size(width, std::max(1.f, ctx.u(2.f))));
            sep->setAnchorPoint(Vec2::ZERO);
            sep->setPosition(Vec2::ZERO);
            sep->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            sep->setBackGroundColor(Color3B(68, 68, 68));
            return sep;
        }

#if KRKR2_USE_COCOS_UI
        Layout *createMenuListRow(const LayoutCtx &ctx, const char *panelName,
                                  const char *btnName, const char *titleName,
                                  const char *titleText, const char *iconPath,
                                  const char *badgeText = nullptr,
                                  const Color3B &iconColor = Color3B::WHITE) {
            auto *panel = Layout::create();
            panel->setName(panelName);
            panel->setContentSize(ctx.size);
            panel->setTouchEnabled(true);

            auto *btn = Button::create("img/empty.png", "img/white.png",
                                       "img/empty.png");
            btn->setName(btnName);
            btn->setScale9Enabled(true);
            setWidgetSize(btn, ctx.size);
            btn->setAnchorPoint(Vec2::ZERO);
            btn->setPosition(Vec2::ZERO);
            btn->setOpacity(64);
            panel->addChild(btn);
            bindFill(btn);

            const float iconSide = std::min(ctx.u(72.f), ctx.h() * 0.78f);
            const float iconCenterX = ctx.u(50.f);
            if(iconPath) {
                auto *icon = ImageView::create(iconPath);
                setWidgetSize(icon, Size(iconSide, iconSide));
                icon->setAnchorPoint(Vec2(0.5f, 0.5f));
                icon->setPosition(Vec2(iconCenterX, ctx.h() * 0.5f));
                icon->setColor(iconColor);
                panel->addChild(icon);
            }
            if(badgeText) {
                auto *badge = Text::create(badgeText, kDefaultFont,
                                           ctx.font(40.f, ctx.h()));
                badge->setTextColor(Color4B::WHITE);
                badge->enableOutline(Color4B(0, 0, 0, 220), 1);
                badge->setAnchorPoint(Vec2(0.5f, 0.5f));
                badge->setPosition(
                    Vec2(iconCenterX + iconSide * 0.34f, ctx.h() * 0.32f));
                panel->addChild(badge);
            }

            const float titleX = ctx.u(104.f);
            const float titleW =
                std::max(ctx.u(60.f), ctx.w() - titleX - ctx.u(16.f));
            auto *title =
                Text::create(titleText, kDefaultFont, ctx.font(50.f, ctx.h()));
            title->setName(titleName);
            title->setTextColor(Color4B(212, 212, 212, 255));
            title->setAnchorPoint(Vec2(0.f, 0.5f));
            title->setPosition(Vec2(titleX, ctx.h() * 0.5f));
            title->setTextAreaSize(Size(titleW, 0.f));
            panel->addChild(title);

            return panel;
        }
#endif

        Layout *createFileManageMenuRow(const LayoutCtx &ctx,
                                        const char *panelName,
                                        const char *btnName,
                                        const char *titleName,
                                        const char *titleText) {
            auto *panel = Layout::create();
            panel->setName(panelName);
            panel->setContentSize(ctx.size);
            panel->setTouchEnabled(true);

            auto *btn = Button::create("img/empty.png", "img/white.png",
                                       "img/empty.png");
            btn->setName(btnName);
            btn->setScale9Enabled(true);
            setWidgetSize(btn, ctx.size);
            btn->setAnchorPoint(Vec2::ZERO);
            btn->setPosition(Vec2::ZERO);
            btn->setOpacity(64);
            panel->addChild(btn);
            bindFill(btn);

            auto *title =
                Text::create(titleText, kDefaultFont, ctx.font(50.f, ctx.h()));
            title->setName(titleName);
            title->setTextColor(Color4B(212, 212, 212, 255));
            title->setAnchorPoint(Vec2(0.f, 0.5f));
            title->setPosition(Vec2(ctx.u(48.f), ctx.h() * 0.5f));
            panel->addChild(title);

            auto *sep = createRowSeparator(ctx, ctx.w());
            panel->addChild(sep);
            bindHorizontalFill(sep, 0.f, 0.f,
                               LayoutComponent::VerticalEdge::Bottom, 0.f);

            return panel;
        }

    } // namespace

    Widget *createListItem(const Size &size, float scale) {
        LayoutCtx ctx{ Size(size.width > 0.f ? size.width
                                             : kCanvasListItemW * scale,
                            size.height),
                       scale };
        const float rowH = ctx.rowH(96.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), rowH));

        auto *item =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        item->setName("item");
        item->setScale9Enabled(true);
        setWidgetSize(item, root->getContentSize());
        item->setAnchorPoint(Vec2(0.5f, 0.5f));
        item->setPosition(root->getContentSize() / 2);
        item->setTitleFontName(kDefaultFont);
        item->setTitleFontSize(ctx.font(54.f, rowH));
        item->setTitleColor(Color3B::WHITE);
        root->addChild(item);
        bindFill(item);

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createSelectListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float rowH = ctx.rowH(96.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), rowH));

        auto *text = Text::create("", kDefaultFont, ctx.font(56.f, rowH));
        text->setName("text");
        text->ignoreContentAdaptWithSize(false);
        text->setContentSize(root->getContentSize());
        text->setTextHorizontalAlignment(TextHAlignment::CENTER);
        text->setTextVerticalAlignment(TextVAlignment::CENTER);
        text->setAnchorPoint(Vec2::ZERO);
        text->setPosition(Vec2::ZERO);
        text->setTextColor(Color4B::WHITE);
        root->addChild(text);
        bindFill(text);

        return root;
    }

    Widget *createComctrlSelectListItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(160.f);
        const float padX = ctx.u(16.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        auto *highlight = createHighlightPanel(itemCtx);
        root->addChild(highlight);
        bindFill(highlight);

        auto *title = Text::create("", kDefaultFont, ctx.font(54.f));
        title->setName("title");
        title->setAnchorPoint(Vec2(0.f, 1.f));
        title->setPosition(Vec2(padX, itemH - ctx.u(14.f)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);
        bindDock(title, LayoutComponent::HorizontalEdge::Left, padX,
                 LayoutComponent::VerticalEdge::Top, ctx.u(14.f));

        auto *selected = Text::create("", kDefaultFont, ctx.font(46.f));
        selected->setName("selected");
        selected->setAnchorPoint(Vec2(1.f, 0.f));
        selected->setPosition(Vec2(ctx.w() - padX, ctx.u(12.f)));
        selected->setTextColor(Color4B(196, 196, 196, 255));
        root->addChild(selected);
        bindDock(selected, LayoutComponent::HorizontalEdge::Right, padX,
                 LayoutComponent::VerticalEdge::Bottom, ctx.u(12.f));

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createCheckBoxItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96.f);
        const float padX = ctx.u(16.f);
        const float boxSide = std::min(ctx.u(72.f), itemH - ctx.u(16.f));
        const float titleH = std::max(ctx.u(40.f), itemH - ctx.u(16.f));
        const float titleW =
            std::max(ctx.u(80.f), ctx.w() - boxSide - padX * 3.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        auto *highlight = createHighlightPanel(itemCtx);
        root->addChild(highlight);
        bindFill(highlight);

        auto *title = Text::create("", kDefaultFont, ctx.font(54.f, itemH));
        title->setName("title");
        title->ignoreContentAdaptWithSize(false);
        title->setContentSize(Size(titleW, titleH));
        title->setTextVerticalAlignment(TextVAlignment::CENTER);
        title->setAnchorPoint(Vec2(0.f, 0.5f));
        title->setPosition(Vec2(padX, itemH * 0.5f));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);
        bindHorizontalFill(title, padX, boxSide + padX * 2.f,
                           LayoutComponent::VerticalEdge::Center);

        auto *checkboxPanel = Layout::create();
        checkboxPanel->setContentSize(Size(boxSide, boxSide));
        checkboxPanel->setAnchorPoint(Vec2(1.f, 0.5f));
        checkboxPanel->setPosition(Vec2(ctx.w() - padX, itemH * 0.5f));
        root->addChild(checkboxPanel);
        bindDock(checkboxPanel, LayoutComponent::HorizontalEdge::Right, padX,
                 LayoutComponent::VerticalEdge::Center);

        auto *checkbox = CheckBox::create(
            "img/CheckBox_Normal.png", "img/CheckBox_Press.png",
            "img/CheckBoxNode_Normal.png", "img/CheckBox_Disable.png",
            "img/empty.png");
        checkbox->setName("checkbox");
        setWidgetSize(checkbox, Size(boxSide, boxSide));
        checkbox->setAnchorPoint(Vec2(0.5f, 0.5f));
        checkbox->setPosition(Vec2(boxSide * 0.5f, boxSide * 0.5f));
        checkboxPanel->addChild(checkbox);

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createSubDirItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96.f);
        const float padX = ctx.u(16.f);
        const float iconSide = std::min(ctx.u(72.f), itemH - ctx.u(16.f));
        const float titleH = std::max(ctx.u(40.f), itemH - ctx.u(16.f));
        const float titleW =
            std::max(ctx.u(80.f), ctx.w() - iconSide - padX * 3.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        LayoutCtx itemCtx{ root->getContentSize(), scale };
        auto *highlight = createHighlightPanel(itemCtx);
        root->addChild(highlight);
        bindFill(highlight);

        auto *title = Text::create("", kDefaultFont, ctx.font(54.f, itemH));
        title->setName("title");
        title->ignoreContentAdaptWithSize(false);
        title->setContentSize(Size(titleW, titleH));
        title->setTextVerticalAlignment(TextVAlignment::CENTER);
        title->setAnchorPoint(Vec2(0.f, 0.5f));
        title->setPosition(Vec2(padX, itemH * 0.5f));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);
        bindHorizontalFill(title, padX, iconSide + padX * 2.f,
                           LayoutComponent::VerticalEdge::Center);

        auto *dirIcon = createRightChevron(ctx, "dir_icon", iconSide);
        dirIcon->setAnchorPoint(Vec2(1.f, 0.5f));
        dirIcon->setPosition(Vec2(ctx.w() - padX, itemH * 0.5f));
        root->addChild(dirIcon);
        bindDock(dirIcon, LayoutComponent::HorizontalEdge::Right, padX,
                 LayoutComponent::VerticalEdge::Center);

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createSliderIconItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(220.f);
        const float padX = ctx.u(16.f);
        const float rightColW = ctx.u(160.f);
        const float panelH = itemH - ctx.u(84.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        auto *title = Text::create("", kDefaultFont, ctx.font(54.f));
        title->setName("title");
        title->setAnchorPoint(Vec2(0.f, 0.5f));
        title->setPosition(Vec2(padX, itemH - ctx.u(42.f)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        const Size panelSize(std::max(ctx.u(240.f), ctx.w() - rightColW),
                             panelH);
        auto *panel = Layout::create();
        panel->setContentSize(panelSize);
        panel->setAnchorPoint(Vec2::ZERO);
        panel->setPosition(Vec2::ZERO);
        root->addChild(panel);

        const Size resetSize(ctx.u(140.f),
                             std::min(ctx.u(68.f), panelH * 0.6f));
        auto *reset = Button::create("Default/Button_Normal.png",
                                     "Default/Button_Press.png",
                                     "Default/Button_Disable.png");
        reset->setName("reset");
        reset->setScale9Enabled(true);
        setWidgetSize(reset, resetSize);
        reset->setTitleFontName(kDefaultFont);
        reset->setTitleText("preference_reset");
        reset->setTitleFontSize(ctx.font(40.f, resetSize.height));
        reset->setTitleColor(Color3B(40, 40, 40));
        reset->setAnchorPoint(Vec2(0.5f, 0.5f));
        reset->setPosition(Vec2(padX + resetSize.width * 0.5f, panelH * 0.5f));
        panel->addChild(reset);

        const float sliderLeft = padX + resetSize.width + ctx.u(36.f);
        const float sliderRight = panelSize.width - ctx.u(28.f);
        const float sliderW = std::max(ctx.u(80.f), sliderRight - sliderLeft);
        auto *slider =
            Slider::create("Default/Slider_Back.png", "img/circle_white.png");
        slider->loadProgressBarTexture("Default/Slider_PressBar.png");
        slider->setName("slider");
        slider->setScale9Enabled(true);
        slider->setContentSize(Size(sliderW, std::max(6.f, ctx.u(14.f))));
        if(auto *ball = slider->getSlidBallRenderer()) {
            ball->setScale(std::max(0.25f, ctx.u(42.f) / 72.f));
        }
        slider->setAnchorPoint(Vec2(0.5f, 0.5f));
        slider->setPosition(Vec2(sliderLeft + sliderW * 0.5f, panelH * 0.5f));
        panel->addChild(slider);

        auto *icon = Widget::create();
        icon->setName("icon");
        icon->setAnchorPoint(Vec2::ZERO);
        icon->setPosition(Vec2(ctx.w() - rightColW * 0.5f, itemH * 0.5f));
        root->addChild(icon);

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createSliderTextItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(220.f);
        const float padX = ctx.u(16.f);
        const float rightColW = ctx.u(160.f);
        const float panelH = itemH - ctx.u(84.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        auto *title = Text::create("", kDefaultFont, ctx.font(54.f));
        title->setName("title");
        title->setAnchorPoint(Vec2(0.f, 0.5f));
        title->setPosition(Vec2(padX, itemH - ctx.u(42.f)));
        title->setTextColor(Color4B::WHITE);
        root->addChild(title);

        const Size panelSize(std::max(ctx.u(240.f), ctx.w() - rightColW),
                             panelH);
        auto *panel = Layout::create();
        panel->setContentSize(panelSize);
        panel->setAnchorPoint(Vec2::ZERO);
        panel->setPosition(Vec2::ZERO);
        root->addChild(panel);

        const Size resetSize(ctx.u(140.f),
                             std::min(ctx.u(68.f), panelH * 0.6f));
        auto *reset = Button::create("Default/Button_Normal.png",
                                     "Default/Button_Press.png",
                                     "Default/Button_Disable.png");
        reset->setName("reset");
        reset->setScale9Enabled(true);
        setWidgetSize(reset, resetSize);
        reset->setTitleFontName(kDefaultFont);
        reset->setTitleText("preference_reset");
        reset->setTitleFontSize(ctx.font(40.f, resetSize.height));
        reset->setTitleColor(Color3B(40, 40, 40));
        reset->setAnchorPoint(Vec2(0.5f, 0.5f));
        reset->setPosition(Vec2(padX + resetSize.width * 0.5f, panelH * 0.5f));
        panel->addChild(reset);

        const float sliderLeft = padX + resetSize.width + ctx.u(36.f);
        const float sliderRight = panelSize.width - ctx.u(28.f);
        const float sliderW = std::max(ctx.u(80.f), sliderRight - sliderLeft);
        auto *slider =
            Slider::create("Default/Slider_Back.png", "img/circle_white.png");
        slider->loadProgressBarTexture("Default/Slider_PressBar.png");
        slider->setName("slider");
        slider->setScale9Enabled(true);
        slider->setContentSize(Size(sliderW, std::max(6.f, ctx.u(14.f))));
        if(auto *ball = slider->getSlidBallRenderer()) {
            ball->setScale(std::max(0.25f, ctx.u(42.f) / 72.f));
        }
        slider->setAnchorPoint(Vec2(0.5f, 0.5f));
        slider->setPosition(Vec2(sliderLeft + sliderW * 0.5f, panelH * 0.5f));
        panel->addChild(slider);

        auto *text = Text::create("100%", kDefaultFont, ctx.font(48.f));
        text->setName("text");
        text->setAnchorPoint(Vec2(1.f, 0.5f));
        text->setPosition(Vec2(ctx.w() - padX, panelH * 0.5f));
        text->setTextColor(Color4B::WHITE);
        root->addChild(text);

        auto *sep = createRowSeparator(ctx, ctx.w());
        root->addChild(sep);
        bindHorizontalFill(sep, 0.f, 0.f, LayoutComponent::VerticalEdge::Bottom,
                           0.f);

        return root;
    }

    Widget *createDeletableItem(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float itemH = ctx.rowH(96.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(ctx.w(), itemH));

        auto *scrollview = ScrollView::create();
        scrollview->setName("scrollview");
        scrollview->setDirection(ScrollView::Direction::HORIZONTAL);
        scrollview->setContentSize(root->getContentSize());
        scrollview->setInnerContainerSize(root->getContentSize());
        scrollview->setAnchorPoint(Vec2::ZERO);
        scrollview->setPosition(Vec2::ZERO);
        scrollview->setTouchEnabled(true);
        root->addChild(scrollview);
        bindFill(scrollview);

        const float delSide = std::min(ctx.u(60.f), itemH * 0.8f);
        auto *deleteIcon =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        deleteIcon->setName("delete");
        setWidgetSize(deleteIcon, Size(delSide, delSide));
        deleteIcon->setAnchorPoint(Vec2(0.f, 0.5f));
        deleteIcon->setPosition(Vec2(ctx.w(), itemH * 0.5f));
        scrollview->addChild(deleteIcon);

        return root;
    }

    Widget *createFileManageMenu(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float titleBarH = std::min(ctx.u(96.f), ctx.h() * 0.16f);
        const float rowH = ctx.u(96.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *bg = Layout::create();
        bg->setContentSize(ctx.size);
        bg->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bg->setBackGroundColor(Color3B(42, 42, 42));
        bg->setAnchorPoint(Vec2::ZERO);
        bg->setPosition(Vec2::ZERO);
        root->addChild(bg);
        bindFill(bg);

        auto *title = Text::create("file_operate_menu_text", kDefaultFont,
                                   ctx.font(54.f, titleBarH));
        title->setName("title");
        title->setTextColor(Color4B(212, 212, 212, 255));
        title->setAnchorPoint(Vec2(0.5f, 0.5f));
        title->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() - titleBarH * 0.5f));
        root->addChild(title);

        auto *divider = Layout::create();
        divider->setContentSize(Size(ctx.w(), std::max(1.f, ctx.u(2.f))));
        divider->setAnchorPoint(Vec2::ZERO);
        divider->setPosition(Vec2(0.f, ctx.h() - titleBarH));
        divider->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        divider->setBackGroundColor(Color3B(120, 120, 120));
        root->addChild(divider);

        auto *list = ListView::create();
        list->setName("list");
        list->setDirection(ScrollView::Direction::VERTICAL);
        list->setContentSize(Size(ctx.w(), std::max(0.f, ctx.h() - titleBarH)));
        list->setAnchorPoint(Vec2::ZERO);
        list->setPosition(Vec2::ZERO);
        list->setClippingEnabled(true);
        list->setBounceEnabled(true);
        list->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(list);

        const Size rowSize(ctx.w(), rowH);
        LayoutCtx rowCtx{ rowSize, scale };
        const struct {
            const char *p, *b, *t, *txt;
        } rows[] = {
            { "unselect", "btnUnselect", "titleUnselect", "filemgr_unselect" },
            { "view", "btnView", "titleView", "filemgr_browse" },
            { "paste", "btnPaste", "titlePaste", "filemgr_paste" },
            { "copy", "btnCopy", "titleCopy", "filemgr_copy" },
            { "unpack", "btnUnpack", "titleUnpack", "filemgr_unpack" },
            { "rename", "btnRename", "titleRename", "filemgr_rename" },
            { "cut", "btnCut", "titleCut", "filemgr_cut" },
            { "delete", "btnDelete", "titleDelete", "filemgr_delete" },
            { "sendto", "btnSendTo", "titleSendTo", "filemgr_sendto" },
        };
        for(const auto &r : rows) {
            list->pushBackCustomItem(
                createFileManageMenuRow(rowCtx, r.p, r.b, r.t, r.txt));
        }

        return root;
    }

    Widget *createListViewPopup(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Layout::create();
        root->setContentSize(ctx.size);
        root->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        root->setBackGroundColor(Color3B(42, 42, 42));

        auto *listView = ListView::create();
        listView->setName("list");
        listView->setDirection(ListView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setClippingEnabled(true);
        listView->setContentSize(ctx.size);
        listView->setAnchorPoint(Vec2::ZERO);
        listView->setPosition(Vec2::ZERO);
        listView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        listView->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(listView);
        bindFill(listView);

        return root;
    }

    Widget *createGameMenu(const Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float barH = ctx.h();
        const Size btnSize(ctx.w() / 5.f, barH);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const float handlerSide = ctx.u(48.f);
        auto *handler = ImageView::create("img/menu_handler.png");
        handler->setName("handler");
        setWidgetSize(handler, Size(handlerSide, handlerSide));
        handler->setAnchorPoint(Vec2::ZERO);
        handler->setPosition(Vec2(ctx.w() - handlerSide, barH));
        handler->setOpacity(38);
        handler->setTouchEnabled(true);
        root->addChild(handler);

        // 下延衬底：上拖弹跳时不露出底部黑缝（对应 GameMenu.csd Panel_1）
        auto *lowerPad = Layout::create();
        lowerPad->setContentSize(Size(ctx.w(), barH * 2.f));
        lowerPad->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        lowerPad->setBackGroundColor(Color3B(42, 42, 42));
        lowerPad->setAnchorPoint(Vec2(0.f, 1.f));
        lowerPad->setPosition(Vec2(0.f, barH));
        lowerPad->setTouchEnabled(true);
        root->addChild(lowerPad);

        auto *panel2 = Layout::create();
        panel2->setContentSize(ctx.size);
        panel2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel2->setBackGroundColor(Color3B(42, 42, 42));
        panel2->setAnchorPoint(Vec2::ZERO);
        panel2->setPosition(Vec2::ZERO);
        panel2->setTouchEnabled(true);
        root->addChild(panel2);

        const float icon64Side = std::min(ctx.u(64.f), barH * 0.6f);
        const float icon72Side = std::min(ctx.u(72.f), barH * 0.65f);
        const Size icon64(icon64Side, icon64Side);
        const Size icon72(icon72Side, icon72Side);
        const Size iconKeyboard(icon64Side, icon64Side * (45.f / 64.f));

        auto *btnGameMenu = createIconButton("img/empty.png", "img/white.png",
                                             btnSize, Vec2::ZERO, Vec2::ZERO,
                                             "img/menu_icon.png", icon64);
        btnGameMenu->setName("btn_gamemenu");
        panel2->addChild(btnGameMenu);

        auto *btnWindow = createIconButton(
            "img/empty.png", "img/white.png", btnSize,
            Vec2(btnSize.width, barH * 0.5f), Vec2(0.f, 0.5f),
            "img/windows_icon.png", icon72, Color3B(208, 208, 208));
        btnWindow->setName("btn_window");
        panel2->addChild(btnWindow);

        auto *btnMouse =
            createIconButton("img/empty.png", "img/white.png", btnSize,
                             Vec2(btnSize.width * 2.f, barH * 0.5f),
                             Vec2(0.f, 0.5f), nullptr, Size::ZERO);
        btnMouse->setName("btn_mousemode");
        auto *iconTouch = ImageView::create("img/touch_icon.png");
        iconTouch->setName("icon_touch");
        setWidgetSize(iconTouch, icon64);
        iconTouch->setAnchorPoint(Vec2(0.5f, 0.5f));
        iconTouch->setPosition(Vec2(btnSize.width * 0.5f, barH * 0.5f));
        iconTouch->setColor(Color3B(208, 208, 208));
        btnMouse->addChild(iconTouch);
        auto *iconMouse = ImageView::create("img/mouse_icon.png");
        iconMouse->setName("icon_mouse");
        setWidgetSize(iconMouse, icon64);
        iconMouse->setAnchorPoint(Vec2(0.5f, 0.5f));
        iconMouse->setPosition(Vec2(btnSize.width * 0.5f, barH * 0.5f));
        iconMouse->setColor(Color3B(208, 208, 208));
        btnMouse->addChild(iconMouse);
        panel2->addChild(btnMouse);

        auto *btnKeyboard = createIconButton(
            "img/empty.png", "img/white.png", btnSize,
            Vec2(btnSize.width * 3.f, barH * 0.5f), Vec2(0.f, 0.5f),
            "img/keyboard_icon.png", iconKeyboard, Color3B(208, 208, 208));
        btnKeyboard->setName("btn_keyboard");
        panel2->addChild(btnKeyboard);

        auto *btnExit = createIconButton(
            "img/empty.png", "img/white.png", btnSize,
            Vec2(ctx.w() - btnSize.width, barH * 0.5f), Vec2(0.f, 0.5f),
            "img/exit_icon.png", icon64, Color3B(160, 160, 160));
        btnExit->setName("btn_exit");
        panel2->addChild(btnExit);

        auto *topLine = Layout::create();
        topLine->setContentSize(Size(ctx.w(), std::max(2.f, ctx.u(3.f))));
        topLine->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        topLine->setBackGroundColor(Color3B(161, 161, 161));
        topLine->setAnchorPoint(Vec2(0.f, 1.f));
        topLine->setPosition(Vec2(0.f, barH));
        root->addChild(topLine);

        return root;
    }

#if KRKR2_USE_COCOS_UI
    Widget *createMenuList(const Size &size, float scale) {
        LayoutCtx ctx{
            Size(size.width > 0.f ? size.width : kCanvasMenuListW * scale,
                 size.height > 0.f ? size.height : kCanvasMenuListH * scale),
            scale
        };
        const float rowH = ctx.u(96.f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *bg = Layout::create();
        bg->setContentSize(ctx.size);
        bg->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bg->setBackGroundColor(Color3B(42, 42, 42));
        bg->setAnchorPoint(Vec2::ZERO);
        bg->setPosition(Vec2::ZERO);
        bg->setTouchEnabled(true);
        root->addChild(bg);
        bindFill(bg);

        // 左边缘细分隔线
        auto *leftBorder = Layout::create();
        leftBorder->setContentSize(Size(std::max(1.f, ctx.u(3.f)), ctx.h()));
        leftBorder->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        leftBorder->setBackGroundColor(Color3B(96, 96, 96));
        leftBorder->setAnchorPoint(Vec2::ZERO);
        leftBorder->setPosition(Vec2::ZERO);
        root->addChild(leftBorder);

        const Size rowSize(ctx.w(), rowH);
        LayoutCtx rowCtx{ rowSize, scale };

        // 与 MenuList.csd 一致：useless_0 保持隐藏，仅保留节点名兼容 NodeMap
        // 查找
        auto *rotateRow = createMenuListRow(
            rowCtx, "useless_0", "btnRotate", "titleRotate", "menu_rotate",
            "img/circle_arrow.png", nullptr, Color3B(192, 192, 192));
        rotateRow->setAnchorPoint(Vec2::ZERO);
        rotateRow->setPosition(Vec2(0.f, ctx.h() - rowH));
        rotateRow->setVisible(false);
        root->addChild(rotateRow);

        const float padY = ctx.u(24.f);
        const float listH = std::max(rowH, ctx.h() - padY * 2.f);
        auto *menulist = ListView::create();
        menulist->setName("menulist");
        menulist->setDirection(ScrollView::Direction::VERTICAL);
        menulist->setContentSize(Size(ctx.w(), listH));
        menulist->setAnchorPoint(Vec2::ZERO);
        menulist->setPosition(Vec2(0.f, padY));
        menulist->setClippingEnabled(true);
        menulist->setBounceEnabled(true);
        root->addChild(menulist);
        bindFill(menulist, 0.f, 0.f, padY, padY);

        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "globalPref", "btnGlobalPref", "titleGlobalPref",
            "menu_global_preference", "img/syssetting_btn_ff.png"));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "newLocalPref", "btnNewLocalPref", "titleNewLocalPref",
            "menu_new_local_pref", "img/syssetting_btn_ff.png", "+"));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "localPref", "btnLocalPref", "titleLocalPref",
            "menu_local_pref", "img/syssetting_btn_ff.png", "L"));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "newFolder", "btnNewFolder", "titleNewFolder",
            "menu_new_folder", "img/windows_icon.png", "+",
            Color3B(192, 192, 192)));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "repack", "btnRepack", "titleRepack", "menu_archive_repack",
            "img/menu_icon.png", nullptr, Color3B(192, 192, 192)));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "help", "btnHelp", "titleHelp", "menu_help",
            "img/about_icon.png", nullptr, Color3B(192, 192, 192)));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "about", "btnAbout", "titleAbout", "menu_about",
            "img/about_icon.png", nullptr, Color3B(192, 192, 192)));
        menulist->pushBackCustomItem(createMenuListRow(
            rowCtx, "exit", "btnExit", "titleExit", "menu_exit",
            "img/exit_icon.png", nullptr, Color3B(192, 192, 192)));

        return root;
    }
#endif

#if KRKR2_USE_COCOS_UI
    Widget *createRecentListItem(const Size &size, float scale) {
        const float width =
            size.width > 0.f ? size.width : kCanvasRecentItemW * scale;
        LayoutCtx ctx{ Size(width, size.height), scale };
        const float itemH = ctx.rowH(120.f);
        const float scrollH = itemH * 0.5f;
        const float bottomH = itemH - scrollH;
        const float btnSide = std::min(ctx.u(52.f), bottomH * 0.84f);
        const float btnY = (bottomH - btnSide) * 0.5f;

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(Size(width, itemH));

        auto *scrollview = ScrollView::create();
        scrollview->setName("scrollview");
        scrollview->setDirection(ScrollView::Direction::HORIZONTAL);
        scrollview->setContentSize(Size(width, scrollH));
        scrollview->setInnerContainerSize(Size(width, scrollH));
        scrollview->setAnchorPoint(Vec2::ZERO);
        scrollview->setPosition(Vec2(0.f, itemH - scrollH));
        scrollview->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        scrollview->setBackGroundColor(Color3B(42, 42, 42));
        scrollview->setTouchEnabled(true);
        scrollview->setClippingEnabled(true);
        root->addChild(scrollview);
        bindHorizontalFill(scrollview, 0.f, 0.f,
                           LayoutComponent::VerticalEdge::Top, 0.f);

        const float delPanelSide = std::min(ctx.u(60.f), scrollH);
        auto *panelDelete = Layout::create();
        panelDelete->setName("panel_delete");
        panelDelete->setContentSize(Size(delPanelSide, delPanelSide));
        panelDelete->setAnchorPoint(Vec2(0.f, 0.5f));
        panelDelete->setPosition(Vec2(width, scrollH * 0.5f));
        panelDelete->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panelDelete->setBackGroundColor(Color3B(151, 0, 1));
        panelDelete->setTouchEnabled(true);
        scrollview->addChild(panelDelete);

        auto *btnDelete =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        btnDelete->setName("btn_delete");
        setWidgetSize(btnDelete, Size(delPanelSide, delPanelSide));
        btnDelete->setAnchorPoint(Vec2::ZERO);
        btnDelete->setPosition(Vec2::ZERO);
        panelDelete->addChild(btnDelete);

        auto *prefix = Text::create("", kDefaultFont, ctx.font(34.f, scrollH));
        prefix->setName("prefix");
        prefix->setTextColor(Color4B(140, 140, 140, 255));
        prefix->setAnchorPoint(Vec2::ZERO);
        prefix->setPosition(Vec2(ctx.u(8.f), ctx.u(4.f)));
        scrollview->addChild(prefix);

        auto *path = Text::create("", kDefaultFont, ctx.font(46.f, scrollH));
        path->setName("path");
        path->setTextColor(Color4B::WHITE);
        path->setAnchorPoint(Vec2::ZERO);
        path->setPosition(Vec2::ZERO);
        prefix->addChild(path);

        auto *file = Text::create("", kDefaultFont, ctx.font(34.f, scrollH));
        file->setName("file");
        file->setTextColor(Color4B(140, 140, 140, 255));
        file->setAnchorPoint(Vec2::ZERO);
        file->setPosition(Vec2::ZERO);
        path->addChild(file);

        auto *btnJump =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        btnJump->setName("btn_jump");
        setWidgetSize(btnJump, Size(btnSide, btnSide));
        btnJump->setAnchorPoint(Vec2(0.5f, 0.f));
        btnJump->setPosition(Vec2(width - ctx.u(40.f), btnY));
        btnJump->setFlippedX(true);
        root->addChild(btnJump);
        bindDock(btnJump, LayoutComponent::HorizontalEdge::Right,
                 ctx.u(40.f) - btnSide * 0.5f,
                 LayoutComponent::VerticalEdge::Bottom, btnY);

        auto *btnConf = Button::create("img/syssetting_btn_ff.png",
                                       "img/syssetting_btn_on.png",
                                       "img/syssetting_btn_on.png");
        btnConf->setName("btn_conf");
        setWidgetSize(btnConf, Size(btnSide, btnSide));
        btnConf->setAnchorPoint(Vec2(0.5f, 0.f));
        btnConf->setPosition(Vec2(width - ctx.u(105.f), btnY));
        root->addChild(btnConf);
        bindDock(btnConf, LayoutComponent::HorizontalEdge::Right,
                 ctx.u(105.f) - btnSide * 0.5f,
                 LayoutComponent::VerticalEdge::Bottom, btnY);

        auto *btnPlay = Button::create("img/triangle.png", "img/triangle.png",
                                       "img/triangle.png");
        btnPlay->setName("btn_play");
        setWidgetSize(btnPlay, Size(btnSide, btnSide));
        btnPlay->setAnchorPoint(Vec2(0.5f, 0.f));
        btnPlay->setPosition(Vec2(ctx.u(64.f), btnY));
        btnPlay->setColor(Color3B(180, 180, 180));
        root->addChild(btnPlay);
        bindDock(btnPlay, LayoutComponent::HorizontalEdge::Left,
                 ctx.u(64.f) - btnSide * 0.5f,
                 LayoutComponent::VerticalEdge::Bottom, btnY);

        auto *separator = Layout::create();
        separator->setContentSize(Size(width, std::max(1.f, ctx.u(2.f))));
        separator->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        separator->setBackGroundColor(Color3B(100, 100, 100));
        separator->setAnchorPoint(Vec2::ZERO);
        separator->setPosition(Vec2::ZERO);
        root->addChild(separator);
        bindHorizontalFill(separator, 0.f, 0.f,
                           LayoutComponent::VerticalEdge::Bottom, 0.f);

        return root;
    }
#endif

} // namespace Csd
