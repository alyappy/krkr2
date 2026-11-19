#include "CsdUIFactory.h"

#include "CsdUILayout.h"

#include <algorithm>

#include <2d/CCSprite.h>
#include <cocos/ui/CocosGUI.h>

using namespace cocos2d;
using namespace cocos2d::ui;

namespace Csd {

    namespace {

        ImageView *makeImage(const char *path, const Size &size,
                             const Vec2 &pos,
                             const Vec2 &anchor = Vec2(0.5f, 0.5f),
                             const Color3B &color = Color3B::WHITE) {
            auto *img = ImageView::create(path);
            setWidgetSize(img, size);
            img->setAnchorPoint(anchor);
            img->setPosition(pos);
            img->setColor(color);
            return img;
        }

        /** help/TouchModeTips.csd 画布 960×360 */
        Widget *buildTouchModeTips(const LayoutCtx &ctx) {
            const float sx = ctx.w() / kCanvasAllTipsW;
            const float sy = ctx.h() / 360.f;
            const float s = std::min(sx, sy);

            auto *root = Widget::create();
            root->setContentSize(ctx.size);
            root->setAnchorPoint(Vec2::ZERO);

            const Size icon64(64.f * s, 64.f * s);
            root->addChild(makeImage("img/touch_icon.png", icon64,
                                     Vec2(90.f * sx, 40.f * sy),
                                     Vec2(0.5f, 0.5f), Color3B(208, 208, 208)));
            root->addChild(makeImage("img/mouse_icon.png", icon64,
                                     Vec2(94.f * sx, 213.f * sy),
                                     Vec2(0.5f, 0.5f), Color3B(208, 208, 208)));

            const Size touchSize(64.f * s, 68.f * s);
            root->addChild(makeImage("img/touch1.png", touchSize,
                                     Vec2(200.f * sx, 40.f * sy)));
            root->addChild(makeImage("img/touch2.png", touchSize,
                                     Vec2(220.f * sx, 40.f * sy)));

            root->addChild(makeImage("img/back_btn_off.png", icon64,
                                     Vec2(92.f * sx, 316.f * sy)));
            auto *back2 = makeImage("img/back_btn_off.png", icon64,
                                    Vec2(92.f * sx, 302.f * sy));
            back2->setRotation(-90.f);
            root->addChild(back2);

            auto *divider = Layout::create();
            divider->setContentSize(Size(std::max(2.f, 8.f * s), 62.f * s));
            divider->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            divider->setBackGroundColor(Color3B(192, 192, 192));
            divider->setAnchorPoint(Vec2(0.5f, 0.5f));
            divider->setPosition(Vec2(92.f * sx, 309.f * sy));
            root->addChild(divider);

            root->addChild(makeImage("img/mouse_2btn.png",
                                     Size(43.f * s, 64.f * s),
                                     Vec2(390.f * sx, 40.f * sy)));

            return root;
        }

        /** help/ScreenModeTips.csd 画布 960×180 */
        Widget *buildScreenModeTips(const LayoutCtx &ctx) {
            const float sx = ctx.w() / kCanvasAllTipsW;
            const float sy = ctx.h() / 180.f;
            const float s = std::min(sx, sy);

            auto *root = Widget::create();
            root->setContentSize(ctx.size);
            root->setAnchorPoint(Vec2::ZERO);

            root->addChild(makeImage("img/windows_icon.png",
                                     Size(80.f * s, 80.f * s),
                                     Vec2(62.f * sx, 90.f * sy)));

            auto *outer = Layout::create();
            outer->setContentSize(Size(200.f * s, 120.f * s));
            outer->setAnchorPoint(Vec2(0.f, 0.5f));
            outer->setPosition(Vec2(193.f * sx, 90.f * sy));
            outer->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            outer->setBackGroundColor(Color3B(208, 208, 208));
            root->addChild(outer);

            auto *inner = Layout::create();
            inner->setContentSize(Size(194.f * s, 114.f * s));
            inner->setAnchorPoint(Vec2(0.5f, 0.5f));
            inner->setPosition(Vec2(outer->getContentSize().width * 0.5f,
                                    outer->getContentSize().height * 0.5f));
            inner->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            inner->setBackGroundColor(Color3B(42, 42, 42));
            outer->addChild(inner);

            auto *preview = Layout::create();
            preview->setContentSize(Size(140.f * s, 114.f * s));
            preview->setAnchorPoint(Vec2(0.5f, 0.5f));
            preview->setPosition(Vec2(inner->getContentSize().width * 0.5f,
                                      inner->getContentSize().height * 0.5f));
            preview->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            preview->setBackGroundColor(Color3B(60, 177, 255));
            inner->addChild(preview);

            auto *abc =
                Text::create("ABC", kDefaultFont,
                             std::max(10, static_cast<int>(48.f * s + 0.5f)));
            abc->setAnchorPoint(Vec2(0.5f, 0.5f));
            abc->setPosition(Vec2(preview->getContentSize().width * 0.5f,
                                  preview->getContentSize().height * 0.5f));
            abc->setTextColor(Color4B::BLACK);
            preview->addChild(abc);

            return root;
        }

        Layout *makePauseIcon(const LayoutCtx &ctx, const char *name,
                              float side, const Color3B &color) {
            const Size iconSize(side, side);
            const Size barSize(side * 0.30f, side);
            auto *root = Layout::create();
            root->setName(name);
            root->setContentSize(iconSize);
            root->setAnchorPoint(Vec2(0.5f, 0.5f));

            auto addBar = [&](float xRatio) {
                auto *bar = Layout::create();
                bar->setContentSize(barSize);
                bar->setAnchorPoint(Vec2(0.5f, 0.5f));
                bar->setPosition(
                    Vec2(iconSize.width * xRatio, iconSize.height * 0.5f));
                bar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
                bar->setBackGroundColor(color);
                root->addChild(bar);
            };
            addBar(0.30f);
            addBar(0.70f);
            return root;
        }

        Widget *buildPlayBtn(const LayoutCtx &ctx) {
            const float btnSide = std::min(ctx.h() * 0.9f, ctx.u(160.f));
            const Size btnSize(btnSide, btnSide);
            const Size circleSize(btnSide * 0.78f, btnSide * 0.78f);
            const float iconSide = btnSide * 0.40f;
            const Size iconSize(iconSide, iconSide);
            const Vec2 center(btnSize.width * 0.5f, btnSize.height * 0.5f);

            auto *playBtn = Layout::create();
            playBtn->setName("PlayBtn");
            playBtn->setContentSize(btnSize);
            playBtn->setAnchorPoint(Vec2(0.5f, 0.5f));
            playBtn->setTouchEnabled(true);

            auto *normal =
                makeImage("img/circle_white.png", circleSize, center);
            normal->setName("PlayBtnNormal");
            playBtn->addChild(normal);

            auto *press = makeImage("img/circle_white.png", circleSize, center,
                                    Vec2(0.5f, 0.5f), Color3B(128, 128, 128));
            press->setName("PlayBtnPress");
            playBtn->addChild(press);

            auto *playIconN =
                makeImage("img/triangle.png", iconSize,
                          Vec2(center.x + btnSide * 0.05f, center.y));
            playIconN->setName("PlayIconNormal");
            playBtn->addChild(playIconN);

            auto *playIconP =
                makeImage("img/triangle.png", iconSize,
                          Vec2(center.x + btnSide * 0.05f, center.y),
                          Vec2(0.5f, 0.5f), Color3B(128, 128, 128));
            playIconP->setName("PlayIconPress");
            playBtn->addChild(playIconP);

            auto *pauseN =
                makePauseIcon(ctx, "PauseIconNormal", iconSide, Color3B::WHITE);
            pauseN->setPosition(center);
            playBtn->addChild(pauseN);

            auto *pauseP = makePauseIcon(ctx, "PauseIconPress", iconSide,
                                         Color3B(128, 128, 128));
            pauseP->setPosition(center);
            playBtn->addChild(pauseP);

            return playBtn;
        }

        Text *makeMediaText(const LayoutCtx &ctx, const char *name,
                            float designFont, const Vec2 &pos,
                            const Vec2 &anchor) {
            auto *text =
                Text::create("Text Label", kDefaultFont, ctx.font(designFont));
            text->setName(name);
            text->setAnchorPoint(anchor);
            text->setPosition(pos);
            text->setTextColor(Color4B::WHITE);
            text->enableShadow(Color4B(40, 40, 40, 200), Size(1, -1));
            return text;
        }

        Layout *makeDialogButton(const LayoutCtx &ctx, const char *panelName,
                                 const char *btnName, const Size &btnSize,
                                 int fontSize) {
            const float border = std::max(2.f, ctx.u(4.f));

            auto *btnPanel = Layout::create();
            btnPanel->setName(panelName);
            btnPanel->setTouchEnabled(true);
            btnPanel->setContentSize(btnSize);
            btnPanel->setBackGroundColorType(
                Layout::BackGroundColorType::SOLID);
            btnPanel->setBackGroundColor(Color3B(68, 68, 68));

            const Size innerSize(std::max(1.f, btnSize.width - border * 2.f),
                                 std::max(1.f, btnSize.height - border * 2.f));
            auto *btnPanelInner = Layout::create();
            btnPanelInner->setTouchEnabled(true);
            btnPanelInner->setContentSize(innerSize);
            btnPanelInner->setBackGroundColorType(
                Layout::BackGroundColorType::SOLID);
            btnPanelInner->setBackGroundColor(Color3B(153, 153, 153));
            btnPanelInner->setAnchorPoint(Vec2::ZERO);
            btnPanelInner->setPosition(Vec2(border, border));
            btnPanel->addChild(btnPanelInner);
            bindFill(btnPanelInner, border, border, border, border);

            auto *button =
                Button::create("img/empty.png", "img/gray.png", "img/gray.png");
            button->setName(btnName);
            button->setTitleFontName(kDefaultFont);
            button->setTitleFontSize(fontSize);
            button->setTitleColor(Color3B::BLACK);
            button->setScale9Enabled(true);
            setWidgetSize(button, innerSize);
            button->setAnchorPoint(Vec2::ZERO);
            button->setPosition(Vec2::ZERO);
            button->setZoomScale(0.05f);
            btnPanelInner->addChild(button);
            bindFill(button);

            return btnPanel;
        }

    } // namespace

    Widget *createEmpty(const cocos2d::Size &, float) { return nullptr; }

#if KRKR2_USE_COCOS_UI
    Widget *createMainFileSelector(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float midLineW = std::max(2.f, std::min(ctx.u(10.f), 12.f));
        const cocos2d::Size midLineSize(midLineW, size.height);
        const cocos2d::Size bothSize((size.width - midLineW) * 0.5f,
                                     size.height);

        const auto root = Layout::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(size);
        root->setLayoutType(Layout::Type::HORIZONTAL);

        // 左侧 recentList
        const auto recentList = ListView::create();
        recentList->setName("recentList");
        recentList->setDirection(ScrollView::Direction::VERTICAL);
        recentList->setContentSize(bothSize);
        recentList->setTouchEnabled(true);
        recentList->setBounceEnabled(true);
        recentList->setClippingEnabled(true);
        recentList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        recentList->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        recentList->setBackGroundColorOpacity(255);
        root->addChild(recentList);

        // 中间线
        const auto ml = Layout::create();
        ml->setName("ml");
        ml->setContentSize(midLineSize);
        ml->setTouchEnabled(true);
        ml->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        ml->setBackGroundColor(cocos2d::Color3B(121, 121, 121));
        ml->setBackGroundColorOpacity(255);
        root->addChild(ml);

        // 右侧 fileList
        const auto fileList = Layout::create();
        fileList->setName("fileList");
        fileList->setLayoutType(Layout::Type::VERTICAL);
        fileList->setContentSize(bothSize);
        fileList->setTouchEnabled(true);
        fileList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        fileList->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        fileList->setBackGroundColorOpacity(255);
        root->addChild(fileList);

        return root;
    }
#endif

    Widget *createTableView(const cocos2d::Size &size, float) {
        const auto root = Widget::create();
        root->setAnchorPoint(cocos2d::Vec2::ZERO);
        root->setContentSize(size);

        Layout *table = Layout::create();
        table->setName("table");
        table->setContentSize(size);
        table->setPosition(cocos2d::Vec2::ZERO);
        table->setAnchorPoint(cocos2d::Vec2::ZERO);
        table->setTouchEnabled(true);
        table->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        table->setBackGroundColor(cocos2d::Color3B(42, 42, 42));
        table->setBackGroundColorOpacity(255);
        root->addChild(table);
        bindFill(table);

        return root;
    }

    Widget *createNaviBar(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float sideMargin = ctx.u(16.f);
        const float btnSide = std::min(ctx.u(84.f), ctx.h() * 0.76f);
        const Size btnSize(btnSide, btnSide);
        const float titleX = sideMargin + btnSide + ctx.u(12.f);
        const Size titleSize(std::max(0.f, ctx.w() - titleX * 2.f), ctx.h());
        const float yCenter = ctx.h() * 0.5f;

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto background = Layout::create();
        background->setName("background");
        background->setContentSize(ctx.size);
        background->setTouchEnabled(true);
        background->setAnchorPoint(Vec2::ZERO);
        background->setPosition(Vec2::ZERO);
        background->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        background->setBackGroundColor(Color3B(42, 42, 42));
        background->setBackGroundColorOpacity(255);

        const auto leftBtn =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        leftBtn->setName("left");
        leftBtn->setTouchEnabled(true);
        setWidgetSize(leftBtn, btnSize);
        leftBtn->setPosition(Vec2(sideMargin, yCenter));
        leftBtn->setAnchorPoint(Vec2(0.f, 0.5f));

        const auto titleBtn =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        titleBtn->setName("title");
        titleBtn->setScale9Enabled(true);
        setWidgetSize(titleBtn, titleSize);
        titleBtn->setPosition(Vec2(titleX, yCenter));
        titleBtn->setAnchorPoint(Vec2(0.f, 0.5f));
        titleBtn->setTitleFontName(kDefaultFont);
        titleBtn->setTitleFontSize(ctx.font(58.f, ctx.h()));
        titleBtn->setTitleAlignment(TextHAlignment::CENTER,
                                    TextVAlignment::CENTER);
        titleBtn->setTouchEnabled(true);
        titleBtn->setTitleColor(Color3B(216, 216, 216));

        const auto bottomLine = Layout::create();
        bottomLine->setContentSize(Size(ctx.w(), std::max(1.f, ctx.u(2.f))));
        bottomLine->setAnchorPoint(Vec2::ZERO);
        bottomLine->setPosition(Vec2::ZERO);
        bottomLine->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bottomLine->setBackGroundColor(Color3B(88, 88, 88));

        background->addChild(leftBtn);
        background->addChild(titleBtn);
        background->addChild(bottomLine);
        root->addChild(background);

        bindFill(background);
        bindDock(leftBtn, LayoutComponent::HorizontalEdge::Left, sideMargin,
                 LayoutComponent::VerticalEdge::Center);
        bindHorizontalFill(titleBtn, titleX, titleX,
                           LayoutComponent::VerticalEdge::Center);
        bindHorizontalFill(bottomLine, 0.f, 0.f,
                           LayoutComponent::VerticalEdge::Bottom, 0.f);
        return root;
    }

    Widget *createNaviBarWithMenu(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float sideMargin = ctx.u(16.f);
        const float btnSide = std::min(ctx.u(80.f), ctx.h() * 0.74f);
        const Size btnSize(btnSide, btnSide);
        const float titleX = sideMargin + btnSide + ctx.u(12.f);
        const Size titleSize(std::max(0.f, ctx.w() - titleX * 2.f), ctx.h());
        const float yCenter = ctx.h() * 0.5f;

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto background = Layout::create();
        background->setName("background");
        background->setContentSize(ctx.size);
        background->setTouchEnabled(true);
        background->setAnchorPoint(Vec2::ZERO);
        background->setPosition(Vec2::ZERO);
        background->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        background->setBackGroundColor(Color3B(42, 42, 42));
        background->setBackGroundColorOpacity(255);

        const auto leftBtn =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        leftBtn->setName("left");
        leftBtn->setTouchEnabled(true);
        setWidgetSize(leftBtn, btnSize);
        leftBtn->setPosition(Vec2(sideMargin, yCenter));
        leftBtn->setAnchorPoint(Vec2(0.f, 0.5f));

        const auto titleBtn =
            Button::create("img/empty.png", "img/gray.png", "img/empty.png");
        titleBtn->setName("title");
        titleBtn->setScale9Enabled(true);
        setWidgetSize(titleBtn, titleSize);
        titleBtn->setPosition(Vec2(titleX, yCenter));
        titleBtn->setAnchorPoint(Vec2(0.f, 0.5f));
        titleBtn->setTitleFontName(kDefaultFont);
        titleBtn->setTitleFontSize(ctx.font(58.f, ctx.h()));
        titleBtn->setTitleAlignment(TextHAlignment::CENTER,
                                    TextVAlignment::CENTER);
        titleBtn->setTouchEnabled(true);
        titleBtn->setTitleColor(Color3B(216, 216, 216));

        const auto rightBtn = Button::create(
            "img/menu_icon.png", "img/menu_press.png", "img/menu_icon.png");
        rightBtn->setName("right");
        rightBtn->setTouchEnabled(true);
        setWidgetSize(rightBtn, btnSize);
        rightBtn->setPosition(Vec2(ctx.w() - sideMargin, yCenter));
        rightBtn->setAnchorPoint(Vec2(1.f, 0.5f));

        const auto bottomLine = Layout::create();
        bottomLine->setContentSize(Size(ctx.w(), std::max(1.f, ctx.u(2.f))));
        bottomLine->setAnchorPoint(Vec2::ZERO);
        bottomLine->setPosition(Vec2::ZERO);
        bottomLine->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        bottomLine->setBackGroundColor(Color3B(88, 88, 88));

        background->addChild(leftBtn);
        background->addChild(titleBtn);
        background->addChild(rightBtn);
        background->addChild(bottomLine);
        root->addChild(background);

        bindFill(background);
        bindDock(leftBtn, LayoutComponent::HorizontalEdge::Left, sideMargin,
                 LayoutComponent::VerticalEdge::Center);
        bindHorizontalFill(titleBtn, titleX, titleX,
                           LayoutComponent::VerticalEdge::Center);
        bindDock(rightBtn, LayoutComponent::HorizontalEdge::Right, sideMargin,
                 LayoutComponent::VerticalEdge::Center);
        bindHorizontalFill(bottomLine, 0.f, 0.f,
                           LayoutComponent::VerticalEdge::Bottom, 0.f);

        return root;
    }

    Widget *createFileItem(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float rowH = ctx.rowH(96.f);
        const float marginX = ctx.u(12.f);
        const float marginY = ctx.u(8.f);
        const float iconSide = std::min(ctx.u(72.f), rowH - marginY * 2.f);
        const float underlineH = std::max(1.f, ctx.u(3.f));
        const Size rootSize(ctx.w(), rowH);
        const Size filenameSize(
            std::max(ctx.u(60.f), rootSize.width - iconSide - marginX * 3.f),
            std::max(ctx.u(32.f), rowH - marginY * 2.f));

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setPosition(Vec2::ZERO);
        root->setContentSize(rootSize);

        const auto filename =
            Text::create("", kDefaultFont, ctx.font(54.f, rowH));
        filename->setName("filename");
        filename->ignoreContentAdaptWithSize(false);
        filename->setContentSize(filenameSize);
        filename->setTextVerticalAlignment(TextVAlignment::CENTER);
        filename->setSwallowTouches(false);
        filename->setAnchorPoint(Vec2(0.f, 0.5f));
        filename->setPosition(Vec2(marginX, rowH * 0.5f));
        filename->setTextColor(Color4B::WHITE);

        // underline: 底部横向渐变分隔线（与 FileItem.csd 一致）
        const auto underline = Layout::create();
        underline->setName("underline");
        underline->setAnchorPoint(Vec2::ZERO);
        underline->setContentSize(Size(rootSize.width, underlineH));
        underline->setPosition(Vec2::ZERO);
        underline->setBackGroundColorType(
            Layout::BackGroundColorType::GRADIENT);
        underline->setBackGroundColor(Color3B(200, 200, 200),
                                      Color3B(42, 42, 42));
        underline->setBackGroundColorVector(Vec2(1.f, 0.f));

        // highlight button（整行点击区域）
        const auto highlight =
            Button::create("img/empty.png", "img/white.png", "img/empty.png");
        highlight->setName("highlight");
        highlight->setScale9Enabled(true);
        setWidgetSize(highlight, rootSize);
        highlight->setPosition(Vec2::ZERO);
        highlight->setAnchorPoint(Vec2::ZERO);
        highlight->setOpacity(51);
        highlight->setSwallowTouches(false);

        // select_check checkbox
        const auto checkBox = CheckBox::create(
            "img/CheckBox_Normal.png", "img/CheckBox_Press.png",
            "img/CheckBoxNode_Normal.png", "img/CheckBox_Disable.png",
            "img/empty.png");
        checkBox->setName("select_check");
        setWidgetSize(checkBox, Size(iconSide, iconSide));
        checkBox->setAnchorPoint(Vec2(1.f, 0.5f));
        checkBox->setPosition(Vec2(rootSize.width - marginX, rowH * 0.5f));

        // dir_icon panel（向右箭头）
        const auto dirIcon = createRightChevron(ctx, "dir_icon", iconSide);
        dirIcon->setAnchorPoint(Vec2(1.f, 0.5f));
        dirIcon->setPosition(Vec2(rootSize.width - marginX, rowH * 0.5f));

        root->addChild(filename);
        root->addChild(underline);
        root->addChild(checkBox);
        root->addChild(dirIcon);
        root->addChild(highlight);

        bindHorizontalFill(underline, 0.f, 0.f,
                           LayoutComponent::VerticalEdge::Bottom, 0.f);
        bindDock(checkBox, LayoutComponent::HorizontalEdge::Right, marginX,
                 LayoutComponent::VerticalEdge::Center);
        bindDock(dirIcon, LayoutComponent::HorizontalEdge::Right, marginX,
                 LayoutComponent::VerticalEdge::Center);
        bindFill(highlight);

        return root;
    }

    Widget *createBottomBarTextInput(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.u(10.f);
        const float btnBarH = std::min(ctx.u(84.f), ctx.h() * 0.48f);

        auto *panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setAnchorPoint(Vec2::ZERO);
        panel4->setPosition(Vec2::ZERO);
        panel4->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel4->setBackGroundColor(Color3B(42, 42, 42));
        panel4->setTouchEnabled(true);

        const Size inputSize(std::max(1.f, ctx.w() - pad * 2.f),
                             std::max(1.f, ctx.h() - btnBarH - pad));
        auto *panel14_9 = Layout::create();
        panel14_9->setContentSize(inputSize);
        panel14_9->setAnchorPoint(Vec2::ZERO);
        panel14_9->setPosition(Vec2(pad, btnBarH));
        panel14_9->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel14_9->setBackGroundColor(Color3B(199, 199, 199));
        panel14_9->setTouchEnabled(true);
        panel4->addChild(panel14_9);

        auto *input = TextField::create("Touch to input", kDefaultFont,
                                        ctx.font(52.f, inputSize.height));
        input->setName("input");
        input->ignoreContentAdaptWithSize(false);
        input->setContentSize(inputSize);
        input->setMaxLengthEnabled(true);
        // File names must survive the edit box; the old demo limit of 10
        // truncated them and made OK silently do nothing for missing files.
        input->setMaxLength(255);
        input->setTextColor(Color4B::BLACK);
        input->setAnchorPoint(Vec2(0.5f, 0.5f));
        input->setPosition(
            Vec2(inputSize.width * 0.5f, inputSize.height * 0.5f));
        panel14_9->addChild(input);

        const Size btnSize(std::min(ctx.relW(0.36f), ctx.u(260.f)),
                           std::max(1.f, btnBarH - pad * 1.4f));
        auto *cancelBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        cancelBtn->setName("cancel");
        cancelBtn->setTitleFontName(kDefaultFont);
        cancelBtn->setTitleText("Cancel");
        cancelBtn->setTitleFontSize(ctx.font(50.f, btnSize.height));
        cancelBtn->setTitleColor(Color3B::BLACK);
        cancelBtn->setScale9Enabled(true);
        setWidgetSize(cancelBtn, btnSize);
        cancelBtn->setAnchorPoint(Vec2(0.f, 0.5f));
        cancelBtn->setPosition(Vec2(pad, btnBarH * 0.5f));
        panel4->addChild(cancelBtn);

        auto *okBtn =
            Button::create("img/gray.png", "img/white.png", "img/gray.png");
        okBtn->setName("ok");
        okBtn->setTitleFontName(kDefaultFont);
        okBtn->setTitleText("OK");
        okBtn->setTitleFontSize(ctx.font(50.f, btnSize.height));
        okBtn->setTitleColor(Color3B::BLACK);
        okBtn->setScale9Enabled(true);
        setWidgetSize(okBtn, btnSize);
        okBtn->setAnchorPoint(Vec2(1.f, 0.5f));
        okBtn->setPosition(Vec2(ctx.w() - pad, btnBarH * 0.5f));
        panel4->addChild(okBtn);

        return panel4;
    }

    Widget *createTextPairInput(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const float pad = ctx.u(12.f);
        const float topBarH = std::min(ctx.u(80.f), ctx.h() * 0.22f);
        const float btnSide = std::min(ctx.u(72.f), topBarH * 0.9f);
        const float btnY = ctx.h() - topBarH * 0.5f;
        const float bodyH = std::max(ctx.u(120.f), ctx.h() - topBarH);
        const float input1H = std::min(ctx.u(80.f), bodyH * 0.30f);
        const float input2H =
            std::max(ctx.u(60.f), bodyH - input1H - pad * 3.f);

        const auto panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setTouchEnabled(true);
        panel4->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel4->setBackGroundColor(Color3B(42, 42, 42));
        panel4->setPosition(Vec2::ZERO);
        root->addChild(panel4);
        bindFill(panel4);

        const float fieldW = std::max(1.f, ctx.w() - pad * 2.f);

        const auto panel13 = Layout::create();
        panel13->setContentSize(Size(fieldW, input1H));
        panel13->setTouchEnabled(true);
        panel13->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel13->setBackGroundColor(Color3B(199, 199, 199));
        panel13->setPosition(Vec2(pad, pad * 2.f + input2H));
        panel4->addChild(panel13);

        const auto input1 = TextField::create("Touch to input", kDefaultFont,
                                              ctx.font(52.f, input1H));
        input1->setName("input1");
        input1->ignoreContentAdaptWithSize(false);
        input1->setMaxLength(255);
        input1->setMaxLengthEnabled(true);
        input1->setTextColor(Color4B::BLACK);
        input1->setContentSize(panel13->getContentSize());
        input1->setPosition(Vec2(fieldW * 0.5f, input1H * 0.5f));
        input1->setTouchEnabled(true);
        panel13->addChild(input1);

        const auto panel14 = Layout::create();
        panel14->setContentSize(Size(fieldW, input2H));
        panel14->setTouchEnabled(true);
        panel14->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel14->setBackGroundColor(Color3B(199, 199, 199));
        panel14->setPosition(Vec2(pad, pad));
        panel4->addChild(panel14);

        const auto input2 = TextField::create("Touch to input", kDefaultFont,
                                              ctx.font(52.f, input1H));
        input2->setName("input2");
        input2->ignoreContentAdaptWithSize(false);
        input2->setMaxLength(255);
        input2->setMaxLengthEnabled(true);
        input2->setTextColor(Color4B::BLACK);
        input2->setContentSize(panel14->getContentSize());
        input2->setPosition(Vec2(fieldW * 0.5f, input2H * 0.5f));
        input2->setTouchEnabled(true);
        panel14->addChild(input2);

        const auto cancelBtn =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/CheckBox_Disable.png");
        cancelBtn->setName("cancel");
        setWidgetSize(cancelBtn, Size(btnSide, btnSide));
        cancelBtn->setPosition(Vec2(pad, btnY));
        cancelBtn->setAnchorPoint(Vec2(0.f, 0.5f));
        cancelBtn->setColor(Color3B(214, 15, 15));
        root->addChild(cancelBtn);

        const auto okBtn = Button::create("img/CheckBoxNode_Normal.png",
                                          "img/CheckBoxNode_Press.png",
                                          "img/CheckBox_Disable.png");
        okBtn->setName("ok");
        setWidgetSize(okBtn, Size(btnSide, btnSide));
        okBtn->setPosition(Vec2(ctx.w() - pad, btnY));
        okBtn->setAnchorPoint(Vec2(1.f, 0.5f));
        okBtn->setColor(Color3B(0, 255, 52));
        root->addChild(okBtn);

        return root;
    }

    Widget *createSelectList(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float topBarH = std::min(ctx.u(84.f), ctx.h() * 0.24f);
        const float btnSide = std::min(ctx.u(72.f), topBarH * 0.88f);
        const float btnY = ctx.h() - topBarH * 0.5f;
        const float contentH = std::max(ctx.u(120.f), ctx.h() - topBarH);
        const float sideColW = ctx.u(80.f);
        const float pageW = std::max(ctx.u(120.f), ctx.w() - sideColW * 2.f);
        const float rowH = ctx.u(96.f);

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto panel4 = Layout::create();
        panel4->setContentSize(ctx.size);
        panel4->setAnchorPoint(Vec2::ZERO);
        panel4->setPosition(Vec2::ZERO);
        panel4->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel4->setBackGroundColor(Color3B(42, 42, 42));
        panel4->setTouchEnabled(true);

        // title
        const auto titleLabel =
            Text::create("", kDefaultFont, ctx.font(48.f, topBarH));
        titleLabel->setName("title");
        titleLabel->setPosition(Vec2(ctx.w() * 0.5f, btnY));
        titleLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
        titleLabel->setTextColor(Color4B::WHITE);
        panel4->addChild(titleLabel);

        // Panel_5 - 内容面板（透明，仅做裁剪）
        const auto panel5 = Layout::create();
        panel5->setContentSize(Size(ctx.w(), contentH));
        panel5->setAnchorPoint(Vec2::ZERO);
        panel5->setPosition(Vec2::ZERO);
        panel5->setTouchEnabled(true);
        panel5->setClippingEnabled(true);

        // Panel_17 - 中央选中高亮带
        const auto panel17 = Layout::create();
        panel17->setContentSize(Size(pageW, rowH));
        panel17->setPosition(Vec2(sideColW, contentH * 0.5f));
        panel17->setAnchorPoint(Vec2(0.f, 0.5f));
        panel17->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel17->setBackGroundColor(Color3B::WHITE);
        panel17->setBackGroundColorOpacity(38);
        panel5->addChild(panel17);

        // pageview 区域
        const auto pageViewPanel = Layout::create();
        pageViewPanel->setName("pageview");
        pageViewPanel->setContentSize(Size(pageW, contentH));
        pageViewPanel->setAnchorPoint(Vec2::ZERO);
        pageViewPanel->setPosition(Vec2(sideColW, 0.f));
        pageViewPanel->setTouchEnabled(true);
        panel5->addChild(pageViewPanel);

        // 左箭头（指向中心）
        const auto dirIconLeft =
            createRightChevron(ctx, "dir_icon", std::min(sideColW, rowH));
        dirIconLeft->setAnchorPoint(Vec2(1.f, 0.5f));
        dirIconLeft->setPosition(Vec2(sideColW, contentH * 0.5f));
        panel5->addChild(dirIconLeft);

        // 右箭头（镜像指向中心）
        const auto dirIconRight =
            createRightChevron(ctx, "dir_icon_0", std::min(sideColW, rowH));
        dirIconRight->setAnchorPoint(Vec2(0.5f, 0.5f));
        dirIconRight->setPosition(
            Vec2(ctx.w() - sideColW * 0.5f, contentH * 0.5f));
        dirIconRight->setScaleX(-1.f);
        panel5->addChild(dirIconRight);

        // Cancel 按钮
        const auto cancelButton =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/CheckBox_Disable.png");
        cancelButton->setName("cancel");
        setWidgetSize(cancelButton, Size(btnSide, btnSide));
        cancelButton->setPosition(Vec2(ctx.u(12.f), btnY));
        cancelButton->setAnchorPoint(Vec2(0.f, 0.5f));
        cancelButton->setColor(Color3B(214, 15, 15));
        panel4->addChild(cancelButton);

        // OK 按钮
        const auto okButton = Button::create("img/CheckBoxNode_Normal.png",
                                             "img/CheckBoxNode_Press.png",
                                             "img/CheckBox_Disable.png");
        okButton->setName("ok");
        setWidgetSize(okButton, Size(btnSide, btnSide));
        okButton->setPosition(Vec2(ctx.w() - ctx.u(12.f), btnY));
        okButton->setAnchorPoint(Vec2(1.f, 0.5f));
        okButton->setColor(Color3B(0, 255, 52));
        panel4->addChild(okButton);

        root->addChild(panel4);
        root->addChild(panel5);

        return root;
    }

    Widget *createListView(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

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
        listView->setItemsMargin(0.f);
        root->addChild(listView);
        bindFill(listView);
        return root;
    }

    Widget *createMessageBox(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        // Panel_1（半透明黑色遮罩，与 MessageBox.csd 一致）
        const auto panel1 = Layout::create();
        panel1->setTouchEnabled(true);
        panel1->setContentSize(ctx.size);
        panel1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel1->setBackGroundColor(Color3B::BLACK);
        panel1->setBackGroundColorOpacity(128);
        panel1->setAnchorPoint(Vec2::ZERO);
        panel1->setPosition(Vec2::ZERO);

        // Panel_2（居中深灰对话框）
        const float boxW = std::min(ctx.w() * 0.82f, ctx.u(760.f));
        const float boxH = std::min(ctx.h() * 0.72f, ctx.u(480.f));
        const float border = std::max(2.f, ctx.u(3.f));
        const float titleAreaH = std::min(ctx.u(96.f), boxH * 0.24f);
        const float sepH = std::max(2.f, ctx.u(8.f));
        const float btnBarH = std::min(ctx.u(120.f), boxH * 0.28f);
        const float pad = ctx.u(8.f);

        const auto panel2 = Layout::create();
        panel2->setTouchEnabled(true);
        panel2->setContentSize(Size(boxW, boxH));
        panel2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel2->setBackGroundColor(Color3B(85, 85, 85));
        panel2->setAnchorPoint(Vec2(0.5f, 0.5f));
        panel2->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        panel1->addChild(panel2);

        // Panel_6（内部深色背景）
        const auto panel6 = Layout::create();
        panel6->setTouchEnabled(true);
        panel6->setContentSize(
            Size(boxW - border * 2.f, boxH - border * 2.f - btnBarH));
        panel6->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel6->setBackGroundColor(Color3B(42, 42, 42));
        panel6->setAnchorPoint(Vec2::ZERO);
        panel6->setPosition(Vec2(border, btnBarH + border));
        panel2->addChild(panel6);

        // Panel_3（标题分隔条）
        const auto panel3 = Layout::create();
        panel3->setContentSize(Size(boxW, sepH));
        panel3->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel3->setBackGroundColor(Color3B(85, 85, 85));
        panel3->setAnchorPoint(Vec2::ZERO);
        panel3->setPosition(Vec2(0.f, boxH - titleAreaH - sepH));
        panel2->addChild(panel3);

        // 标题 title
        const auto title =
            Text::create("", kDefaultFont, ctx.font(56.f, titleAreaH));
        title->setName("title");
        title->setTextColor(Color4B::WHITE);
        title->setAnchorPoint(Vec2(0.5f, 0.5f));
        title->setPosition(Vec2(boxW * 0.5f, boxH - titleAreaH * 0.5f));
        panel2->addChild(title);

        // ScrollView text（正文滚动区）
        const float scrollW = std::max(1.f, boxW - pad * 2.f);
        const float scrollH =
            std::max(1.f, boxH - titleAreaH - sepH - btnBarH - pad * 2.f);
        const auto scrollView = ScrollView::create();
        scrollView->setName("text");
        scrollView->setDirection(ScrollView::Direction::VERTICAL);
        scrollView->setTouchEnabled(true);
        scrollView->setClippingEnabled(true);
        scrollView->setContentSize(Size(scrollW, scrollH));
        scrollView->setInnerContainerSize(Size(scrollW, scrollH));
        scrollView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        scrollView->setBackGroundColor(Color3B(32, 32, 32));
        scrollView->setAnchorPoint(Vec2::ZERO);
        scrollView->setPosition(Vec2(pad, btnBarH + pad));
        panel2->addChild(scrollView);

        const float textPad = ctx.u(16.f);
        const auto label = Text::create("", kDefaultFont, ctx.font(48.f));
        label->setName("content");
        label->setTextColor(Color4B::WHITE);
        label->setAnchorPoint(Vec2(0.f, 1.f));
        label->setTextAreaSize(
            Size(std::max(1.f, scrollW - textPad * 2.f), 0.f));
        label->setPosition(Vec2(textPad, scrollH - ctx.u(8.f)));
        scrollView->addChild(label);

        // btnList Panel
        const auto btnList = Layout::create();
        btnList->setTouchEnabled(true);
        btnList->setContentSize(Size(boxW - pad * 2.f, btnBarH - pad));
        btnList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        btnList->setBackGroundColor(Color3B(32, 32, 32));
        btnList->setAnchorPoint(Vec2::ZERO);
        btnList->setPosition(Vec2(pad, pad * 0.5f));
        panel2->addChild(btnList);

        const float btnH = std::max(
            ctx.u(48.f), btnList->getContentSize().height - ctx.u(12.f));
        const float btnW =
            std::min(ctx.u(240.f), btnList->getContentSize().width * 0.45f);
        auto *btnPanel = makeDialogButton(
            ctx, "btn", "btnBody", Size(btnW, btnH), ctx.font(52.f, btnH));
        btnPanel->setAnchorPoint(Vec2::ZERO);
        btnPanel->setPosition(Vec2(
            ctx.u(10.f), (btnList->getContentSize().height - btnH) * 0.5f));
        btnList->addChild(btnPanel);

        root->addChild(panel1);
        bindFill(panel1);
        return root;
    }

    Widget *createProgressBox(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        // Panel_1（半透明黑色遮罩）
        const auto panel1 = Layout::create();
        panel1->setName("Panel_1");
        panel1->setContentSize(ctx.size);
        panel1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel1->setBackGroundColor(Color3B::BLACK);
        panel1->setBackGroundColorOpacity(128);
        panel1->setAnchorPoint(Vec2::ZERO);
        panel1->setPosition(Vec2::ZERO);
        panel1->setTouchEnabled(true);
        root->addChild(panel1);
        bindFill(panel1);

        // Panel_2（居中对话框）
        const float boxW = std::min(ctx.w() * 0.82f, ctx.u(760.f));
        const float boxH = std::min(ctx.h() * 0.80f, ctx.u(600.f));
        const float border = std::max(2.f, ctx.u(3.f));
        const float titleAreaH = std::min(ctx.u(96.f), boxH * 0.18f);
        const float sepH = std::max(2.f, ctx.u(8.f));
        const float btnBarH = std::min(ctx.u(120.f), boxH * 0.22f);
        const float pad = ctx.u(16.f);

        const auto panel2 = Layout::create();
        panel2->setName("Panel_2");
        panel2->setContentSize(Size(boxW, boxH));
        panel2->setTouchEnabled(true);
        panel2->setAnchorPoint(Vec2(0.5f, 0.5f));
        panel2->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        panel2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel2->setBackGroundColor(Color3B(85, 85, 85));
        panel1->addChild(panel2);

        const auto panel6 = Layout::create();
        panel6->setName("Panel_6");
        panel6->setContentSize(
            Size(boxW - border * 2.f, boxH - border * 2.f - btnBarH));
        panel6->setAnchorPoint(Vec2::ZERO);
        panel6->setPosition(Vec2(border, btnBarH + border));
        panel6->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel6->setBackGroundColor(Color3B(42, 42, 42));
        panel2->addChild(panel6);

        const auto panel3 = Layout::create();
        panel3->setContentSize(Size(boxW, sepH));
        panel3->setAnchorPoint(Vec2::ZERO);
        panel3->setPosition(Vec2(0.f, boxH - titleAreaH - sepH));
        panel3->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel3->setBackGroundColor(Color3B(85, 85, 85));
        panel2->addChild(panel3);

        const auto title =
            Text::create("", kDefaultFont, ctx.font(56.f, titleAreaH));
        title->setName("title");
        title->setTextColor(Color4B::WHITE);
        title->setAnchorPoint(Vec2(0.5f, 0.5f));
        title->setPosition(Vec2(boxW * 0.5f, boxH - titleAreaH * 0.5f));
        panel2->addChild(title);

        const float bodyTop = boxH - titleAreaH - sepH - pad;
        const float barW = std::max(1.f, boxW - pad * 2.f);
        const float barH = std::min(ctx.u(60.f), (bodyTop - btnBarH) * 0.24f);
        const float barGap = ctx.u(16.f);

        auto addProgressBar = [&](const char *barName, const char *txtName,
                                  float yTop) {
            auto *trough = Layout::create();
            trough->setContentSize(Size(barW, barH));
            trough->setAnchorPoint(Vec2(0.f, 1.f));
            trough->setPosition(Vec2(pad, yTop));
            trough->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
            trough->setBackGroundColor(Color3B(26, 26, 26));
            panel2->addChild(trough);

            auto *bar = LoadingBar::create("img/white.png");
            bar->setName(barName);
            bar->setScale9Enabled(true);
            setWidgetSize(bar, Size(barW, barH));
            bar->setAnchorPoint(Vec2::ZERO);
            bar->setPosition(Vec2::ZERO);
            bar->setColor(Color3B(96, 168, 224));
            bar->setPercent(0.f);
            trough->addChild(bar);

            auto *lbl = Text::create("", kDefaultFont, ctx.font(44.f, barH));
            lbl->setName(txtName);
            lbl->setTextColor(Color4B::WHITE);
            lbl->enableOutline(Color4B(48, 48, 48, 255), 2);
            lbl->setAnchorPoint(Vec2(0.5f, 0.5f));
            lbl->setPosition(Vec2(barW * 0.5f, barH * 0.5f));
            trough->addChild(lbl);
        };

        addProgressBar("progrss_1", "progress_text_1", bodyTop);
        addProgressBar("progrss_2", "progress_text_2", bodyTop - barH - barGap);

        const float textTop = bodyTop - (barH + barGap) * 2.f;
        const float textH = std::max(ctx.u(40.f), textTop - btnBarH - pad);
        const auto contentText = Text::create("", kDefaultFont, ctx.font(44.f));
        contentText->setName("text");
        contentText->setTextColor(Color4B::WHITE);
        contentText->ignoreContentAdaptWithSize(false);
        contentText->setContentSize(Size(barW, textH));
        contentText->setTextAreaSize(Size(barW, textH));
        contentText->setAnchorPoint(Vec2(0.f, 1.f));
        contentText->setPosition(Vec2(pad, textTop));
        panel2->addChild(contentText);

        const auto btnList = Layout::create();
        btnList->setName("btnList");
        btnList->setContentSize(Size(boxW - pad, btnBarH - ctx.u(8.f)));
        btnList->setAnchorPoint(Vec2::ZERO);
        btnList->setPosition(Vec2(pad * 0.5f, ctx.u(4.f)));
        btnList->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        btnList->setBackGroundColor(Color3B(32, 32, 32));
        panel2->addChild(btnList);

        const float btnH = std::max(
            ctx.u(48.f), btnList->getContentSize().height - ctx.u(12.f));
        const float btnW =
            std::min(ctx.u(240.f), btnList->getContentSize().width * 0.45f);
        auto *btnCell = makeDialogButton(
            ctx, "btnCell", "btn", Size(btnW, btnH), ctx.font(52.f, btnH));
        btnCell->setAnchorPoint(Vec2(0.5f, 0.5f));
        btnCell->setPosition(Vec2(btnList->getContentSize().width * 0.5f,
                                  btnList->getContentSize().height * 0.5f));
        btnList->addChild(btnCell);

        return root;
    }

    Widget *createCheckListDialog(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto panel_20 = Layout::create();
        panel_20->setContentSize(ctx.size);
        panel_20->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel_20->setBackGroundColor(Color3B::BLACK);
        panel_20->setBackGroundColorOpacity(128);
        panel_20->setAnchorPoint(Vec2::ZERO);
        panel_20->setPosition(Vec2::ZERO);
        panel_20->setTouchEnabled(true);

        // Panel_1 中心主面板
        const Size mainSize(ctx.relW(0.9f), ctx.relH(0.9f));
        const float pad = ctx.u(12.f);
        const float titleH = std::min(ctx.u(88.f), mainSize.height * 0.14f);
        const float btnBarH = std::min(ctx.u(112.f), mainSize.height * 0.18f);
        const float listsH =
            std::max(1.f, mainSize.height - titleH - btnBarH - pad * 2.f);

        const auto panel_1 = Layout::create();
        panel_1->setContentSize(mainSize);
        panel_1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        panel_1->setBackGroundColor(Color3B(64, 64, 64));
        panel_1->setAnchorPoint(Vec2(0.5f, 0.5f));
        panel_1->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        panel_1->setTouchEnabled(true);
        panel_20->addChild(panel_1);

        // title 文本
        const auto title =
            Text::create("", kDefaultFont, ctx.font(56.f, titleH));
        title->setName("title");
        title->setAnchorPoint(Vec2(0.f, 0.5f));
        title->setPosition(Vec2(pad, mainSize.height - titleH * 0.5f));
        title->setTextColor(Color4B::WHITE);
        panel_1->addChild(title);

        // 双列列表区域
        const float colW = (mainSize.width - pad * 3.f) * 0.5f;
        const auto list_1 = ListView::create();
        list_1->setName("list_1");
        list_1->setDirection(ListView::Direction::VERTICAL);
        list_1->setContentSize(Size(colW, listsH));
        list_1->setAnchorPoint(Vec2::ZERO);
        list_1->setPosition(Vec2(pad, btnBarH + pad));
        list_1->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list_1->setBackGroundColor(Color3B(42, 42, 42));
        list_1->setClippingEnabled(true);
        list_1->setBounceEnabled(true);
        panel_1->addChild(list_1);

        const auto list_2 = ListView::create();
        list_2->setName("list_2");
        list_2->setDirection(ListView::Direction::VERTICAL);
        list_2->setContentSize(Size(colW, listsH));
        list_2->setAnchorPoint(Vec2::ZERO);
        list_2->setPosition(Vec2(pad * 2.f + colW, btnBarH + pad));
        list_2->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        list_2->setBackGroundColor(Color3B(42, 42, 42));
        list_2->setClippingEnabled(true);
        list_2->setBounceEnabled(true);
        panel_1->addChild(list_2);

        // btn_list 底部按钮栏
        const auto btn_list = ScrollView::create();
        btn_list->setName("btn_list");
        btn_list->setContentSize(Size(mainSize.width, btnBarH));
        btn_list->setInnerContainerSize(Size(mainSize.width, btnBarH));
        btn_list->setAnchorPoint(Vec2(0.5f, 0.f));
        btn_list->setPosition(Vec2(mainSize.width * 0.5f, 0.f));
        btn_list->setDirection(ScrollView::Direction::HORIZONTAL);
        btn_list->setClippingEnabled(true);
        panel_1->addChild(btn_list);

        const float btnH = std::max(ctx.u(48.f), btnBarH - ctx.u(16.f));
        const float btnW = std::min(ctx.u(240.f), mainSize.width * 0.35f);
        auto *btn_cell = makeDialogButton(
            ctx, "btn_cell", "btn", Size(btnW, btnH), ctx.font(52.f, btnH));
        btn_cell->setAnchorPoint(Vec2(0.5f, 0.5f));
        btn_cell->setPosition(Vec2(mainSize.width * 0.5f, btnBarH * 0.5f));
        btn_list->addChild(btn_cell);

        root->addChild(panel_20);
        bindFill(panel_20);
        return root;
    }

    Widget *createMediaPlayerNavi(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.u(16.f);
        const float btnSide = std::min(ctx.u(72.f), ctx.h() * 0.65f);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *naviBar = Layout::create();
        naviBar->setName("NaviBar");
        naviBar->setContentSize(ctx.size);
        naviBar->setAnchorPoint(Vec2::ZERO);
        naviBar->setPosition(Vec2::ZERO);
        naviBar->setTouchEnabled(true);
        naviBar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        naviBar->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(naviBar);
        bindFill(naviBar);

        auto *back =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        back->setName("Back");
        setWidgetSize(back, Size(btnSide, btnSide));
        back->setAnchorPoint(Vec2(0.f, 0.5f));
        back->setPosition(Vec2(pad, ctx.h() * 0.68f));
        naviBar->addChild(back);

        auto *title = makeMediaText(ctx, "Title", 38.f,
                                    Vec2(ctx.w() * 0.5f, ctx.h() * 0.88f),
                                    Vec2(0.5f, 1.f));
        naviBar->addChild(title);

        auto *timeline =
            Slider::create("Default/Slider_Back.png", "img/circle_white.png");
        timeline->loadProgressBarTexture("Default/Slider_PressBar.png");
        timeline->setName("Timeline");
        timeline->setScale9Enabled(true);
        timeline->setContentSize(Size(std::max(1.f, ctx.w() - pad * 2.f),
                                      std::max(6.f, ctx.u(14.f))));
        if(auto *ball = timeline->getSlidBallRenderer()) {
            ball->setScale(std::max(0.25f, ctx.u(36.f) / 72.f));
        }
        timeline->setAnchorPoint(Vec2(0.5f, 0.5f));
        timeline->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.34f));
        naviBar->addChild(timeline);

        auto *playTime = makeMediaText(ctx, "PlayTime", 30.f,
                                       Vec2(pad, ctx.u(4.f)), Vec2(0.f, 0.f));
        naviBar->addChild(playTime);

        auto *remainTime =
            makeMediaText(ctx, "RemainTime", 30.f,
                          Vec2(ctx.w() - pad, ctx.u(4.f)), Vec2(1.f, 0.f));
        naviBar->addChild(remainTime);

        return root;
    }

    Widget *createMediaPlayerBody(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *overlay = Layout::create();
        overlay->setName("Overlay");
        overlay->setContentSize(ctx.size);
        overlay->setAnchorPoint(Vec2::ZERO);
        overlay->setPosition(Vec2::ZERO);
        overlay->setTouchEnabled(true);
        overlay->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        overlay->setBackGroundColor(Color3B::BLACK);
        root->addChild(overlay);
        bindFill(overlay);

        const Size osdSize(std::min(ctx.w() * 0.4f, ctx.u(384.f)),
                           std::min(ctx.h() * 0.3f, ctx.u(192.f)));
        auto *osd = Layout::create();
        osd->setName("OSD");
        osd->setContentSize(osdSize);
        osd->setAnchorPoint(Vec2(0.5f, 0.5f));
        osd->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        osd->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        osd->setBackGroundColor(Color3B(32, 32, 32));
        osd->setBackGroundColorOpacity(191);
        root->addChild(osd);

        auto *osdText = Text::create("Text Label", kDefaultFont,
                                     ctx.font(56.f, osdSize.height));
        osdText->setName("OSDText");
        osdText->setAnchorPoint(Vec2(0.5f, 0.5f));
        osdText->setPosition(Vec2(osdSize.width * 0.5f, osdSize.height * 0.5f));
        osdText->setTextColor(Color4B::WHITE);
        osd->addChild(osdText);

        return root;
    }

    Widget *createMediaPlayerFoot(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *controlBar = Layout::create();
        controlBar->setName("ControlBar");
        controlBar->setContentSize(ctx.size);
        controlBar->setAnchorPoint(Vec2::ZERO);
        controlBar->setPosition(Vec2::ZERO);
        controlBar->setTouchEnabled(true);
        controlBar->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        controlBar->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(controlBar);
        bindFill(controlBar);

        auto *playBtn = buildPlayBtn(ctx);
        playBtn->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        controlBar->addChild(playBtn);

        return root;
    }

    Widget *createAllTips(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };

        const auto root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        const auto listView = ListView::create();
        listView->setName("tipslist");
        listView->setDirection(ScrollView::Direction::VERTICAL);
        listView->setBounceEnabled(true);
        listView->setTouchEnabled(true);
        listView->setContentSize(ctx.size);
        listView->setAnchorPoint(Vec2::ZERO);
        listView->setPosition(Vec2::ZERO);
        listView->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        listView->setBackGroundColor(Color3B(42, 42, 42));
        root->addChild(listView);
        bindFill(listView);

        const float touchH = ctx.relH(360.f / kCanvasAllTipsH);
        const float screenH = ctx.relH(180.f / kCanvasAllTipsH);

        auto *touchTip = Layout::create();
        touchTip->setName("touchTip");
        touchTip->setContentSize(Size(ctx.w(), touchH));
        LayoutCtx touchCtx{ touchTip->getContentSize(), scale };
        touchTip->addChild(buildTouchModeTips(touchCtx));
        listView->pushBackCustomItem(touchTip);

        auto *screenTip = Layout::create();
        screenTip->setName("screenTip");
        screenTip->setContentSize(Size(ctx.w(), screenH));
        LayoutCtx screenCtx{ screenTip->getContentSize(), scale };
        screenTip->addChild(buildScreenModeTips(screenCtx));
        listView->pushBackCustomItem(screenTip);

        const float closeSide = ctx.u(80.f);
        const auto btnClose =
            Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png",
                           "img/Cancel_Press.png");
        btnClose->setName("btn_close");
        setWidgetSize(btnClose, Size(closeSide, closeSide));
        btnClose->setAnchorPoint(Vec2(1.f, 1.f));
        btnClose->setPosition(Vec2(ctx.w() - ctx.u(8.f), ctx.h() - ctx.u(8.f)));
        root->addChild(btnClose);
        bindDock(btnClose, LayoutComponent::HorizontalEdge::Right, ctx.u(8.f),
                 LayoutComponent::VerticalEdge::Top, ctx.u(8.f));

        return root;
    }

    Widget *createWinMgrOverlay(const cocos2d::Size &size, float scale) {
        LayoutCtx ctx{ size, scale };
        const float pad = ctx.u(20.f);
        const float navSide = ctx.u(64.f);
        const float topSide = ctx.u(76.f);
        const Size btnSize(navSide, navSide);
        const Size topBtnSize(topSide, topSide);

        auto *root = Widget::create();
        root->setAnchorPoint(Vec2::ZERO);
        root->setContentSize(ctx.size);

        auto *dim = Layout::create();
        dim->setName("Panel_1");
        dim->setContentSize(ctx.size);
        dim->setAnchorPoint(Vec2(0.5f, 0.5f));
        dim->setPosition(Vec2(ctx.w() * 0.5f, ctx.h() * 0.5f));
        dim->setBackGroundColorType(Layout::BackGroundColorType::SOLID);
        dim->setBackGroundColor(Color3B::BLACK);
        dim->setBackGroundColorOpacity(51);
        dim->setTouchEnabled(true);
        root->addChild(dim);
        bindFill(dim);

        auto *left =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        left->setName("left");
        setWidgetSize(left, btnSize);
        left->setAnchorPoint(Vec2(0.f, 0.5f));
        left->setPosition(Vec2(pad, ctx.h() * 0.5f));
        root->addChild(left);

        auto *right =
            Button::create("img/back_btn_off.png", "img/back_btn_on.png",
                           "img/back_btn_on.png");
        right->setName("right");
        setWidgetSize(right, btnSize);
        right->setScaleX(-1.f);
        right->setAnchorPoint(Vec2(0.5f, 0.5f));
        right->setPosition(
            Vec2(ctx.w() - pad - navSide * 0.5f, ctx.h() * 0.5f));
        root->addChild(right);

        auto *ok =
            Button::create("img/CheckBox_Normal.png", "img/CheckBox_Press.png",
                           "img/CheckBox_Disable.png");
        ok->setName("ok");
        setWidgetSize(ok, topBtnSize);
        ok->setAnchorPoint(Vec2(0.5f, 0.5f));
        ok->setPosition(Vec2(ctx.w() - ctx.u(80.f), ctx.h() - ctx.u(80.f)));
        root->addChild(ok);

        auto *fillscr =
            Button::create("img/CheckBox_Normal.png", "img/CheckBox_Press.png",
                           "img/CheckBox_Disable.png");
        fillscr->setName("fillscr");
        setWidgetSize(fillscr, topBtnSize);
        fillscr->setAnchorPoint(Vec2(0.5f, 0.5f));
        fillscr->setPosition(Vec2(ctx.u(80.f), ctx.h() - ctx.u(80.f)));
        root->addChild(fillscr);

        auto *fillIcon =
            makeImage("img/windows_icon.png", Size(ctx.u(36.f), ctx.u(36.f)),
                      Vec2(topBtnSize.width * 0.5f, topBtnSize.height * 0.5f));
        fillscr->addChild(fillIcon);

        return root;
    }

    Widget *
    createSeperateItem(float width, float lineHeight = 2.0f,
                       const cocos2d::Color4F &color = cocos2d::Color4F::GRAY) {
        Widget *root = Widget::create();
        root->setContentSize(cocos2d::Size(width, lineHeight + 20.f));

        cocos2d::DrawNode *line = cocos2d::DrawNode::create();
        cocos2d::Vec2 from(0.f, 0.f);
        cocos2d::Vec2 to(width, 0.f);
        line->drawLine(from, to, color);

        line->setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE_LEFT);
        line->setPosition(cocos2d::Vec2(0.f, (lineHeight + 20.f) * 0.5f));

        root->addChild(line);
        return root;
    }

} // namespace Csd
