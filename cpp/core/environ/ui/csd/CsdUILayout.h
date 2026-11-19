#pragma once

#include <algorithm>

#include <cocos/ui/CocosGUI.h>
#include <cocos/ui/UILayoutComponent.h>

namespace Csd {

    constexpr const char *kDefaultFont = "NotoSansCJK-Regular.ttc";

    /** CSD 画布参考尺寸 */
    constexpr float kCanvasMenuListW = 720.f;
    constexpr float kCanvasMenuListH = 600.f;
    constexpr float kCanvasNaviBarW = 720.f;
    constexpr float kCanvasNaviBarH = 120.f;
    constexpr float kCanvasComctrlW = 640.f;
    constexpr float kCanvasListViewW = 720.f;
    constexpr float kCanvasListViewH = 960.f;
    constexpr float kCanvasListItemW = 720.f;
    constexpr float kCanvasListItemH = 96.f;
    constexpr float kCanvasSliderIconW = 1280.f;
    constexpr float kCanvasSliderIconH = 240.f;
    constexpr float kCanvasSliderTextW = 640.f;
    constexpr float kCanvasSliderTextH = 240.f;
    constexpr float kCanvasFileManageMenuW = 720.f;
    constexpr float kCanvasFileManageMenuH = 1000.f;
    constexpr float kCanvasAllTipsW = 960.f;
    constexpr float kCanvasAllTipsH = 480.f;
    constexpr float kCanvasGameMenuW = 720.f;
    constexpr float kCanvasGameMenuH = 130.f;
    constexpr float kCanvasFileItemW = 640.f;
    constexpr float kCanvasFileItemH = 96.f;
    constexpr float kCanvasBottomBarW = 720.f;
    constexpr float kCanvasBottomBarH = 200.f;
    constexpr float kCanvasTextPairW = 720.f;
    constexpr float kCanvasTextPairH = 480.f;
    constexpr float kCanvasSelectListW = 720.f;
    constexpr float kCanvasSelectListH = 480.f;
    constexpr float kCanvasMessageBoxW = 720.f;
    constexpr float kCanvasMessageBoxH = 960.f;
    constexpr float kCanvasMediaNaviW = 960.f;
    constexpr float kCanvasMediaNaviH = 120.f;
    constexpr float kCanvasMediaBodyW = 960.f;
    constexpr float kCanvasMediaBodyH = 640.f;
    constexpr float kCanvasMediaFootW = 960.f;
    constexpr float kCanvasMediaFootH = 200.f;
    constexpr float kCanvasWinMgrW = 960.f;
    constexpr float kCanvasWinMgrH = 640.f;
    constexpr float kCanvasRecentItemW = 480.f;
    constexpr float kCanvasRecentItemH = 120.f;

    /**
     * 布局上下文：size 为父级分配区域，scale 为等比 UI 缩放系数。
     * 所有图标、字号、行高、边距一律按 scale 等比换算，避免宽屏拉伸或压扁。
     */
    struct LayoutCtx {
        cocos2d::Size size;
        float scale;

        float s() const { return scale > 0.f ? scale : 1.f; }
        float w() const { return size.width; }
        float h() const { return size.height; }

        /** 按统一 UI 缩放将 CSD 设计像素换算为当前坐标系像素 */
        float u(float designPx) const { return designPx * s(); }

        cocos2d::Size uSize(float designW, float designH) const {
            return { u(designW), u(designH) };
        }

        /** 按容器比例换算（用于分栏或占比面板） */
        float relW(float ratio) const { return size.width * ratio; }
        float relH(float ratio) const { return size.height * ratio; }

        /** 统一等比字号，可选按行高上限裁剪 */
        int font(float designFont, float maxH = 0.f) const {
            float px = designFont * s();
            if(maxH > 0.f) {
                px = std::min(px, maxH * 0.72f);
            }
            return std::max(10, static_cast<int>(px + 0.5f));
        }

        /**
         * 单行列表项高度：父级传 height == 0 时按 designRowH * scale 计算；
         * 父级显式给定高度时直接采用。
         */
        float rowH(float designRowH) const {
            if(size.height > 0.f)
                return size.height;
            return u(designRowH);
        }
    };

    inline void setWidgetSize(cocos2d::ui::Widget *widget,
                              const cocos2d::Size &size) {
        if(!widget)
            return;
        widget->ignoreContentAdaptWithSize(false);
        widget->setContentSize(size);
    }

    inline cocos2d::ui::LayoutComponent *
    bindFill(cocos2d::Node *node, float left = 0.f, float right = 0.f,
             float top = 0.f, float bottom = 0.f) {
        auto *lc = cocos2d::ui::LayoutComponent::bindLayoutComponent(node);
        lc->setHorizontalEdge(
            cocos2d::ui::LayoutComponent::HorizontalEdge::Center);
        lc->setVerticalEdge(cocos2d::ui::LayoutComponent::VerticalEdge::Center);
        lc->setStretchWidthEnabled(true);
        lc->setStretchHeightEnabled(true);
        lc->setLeftMargin(left);
        lc->setRightMargin(right);
        lc->setTopMargin(top);
        lc->setBottomMargin(bottom);
        return lc;
    }

    inline cocos2d::ui::LayoutComponent *
    bindHorizontalFill(cocos2d::Node *node, float left, float right,
                       cocos2d::ui::LayoutComponent::VerticalEdge vEdge,
                       float vMargin = 0.f) {
        auto *lc = cocos2d::ui::LayoutComponent::bindLayoutComponent(node);
        lc->setHorizontalEdge(
            cocos2d::ui::LayoutComponent::HorizontalEdge::Center);
        lc->setStretchWidthEnabled(true);
        lc->setLeftMargin(left);
        lc->setRightMargin(right);
        if(vEdge == cocos2d::ui::LayoutComponent::VerticalEdge::Center) {
            lc->setVerticalEdge(
                cocos2d::ui::LayoutComponent::VerticalEdge::None);
            lc->setPositionPercentYEnabled(true);
            lc->setPositionPercentY(0.5f);
        } else {
            lc->setVerticalEdge(vEdge);
            if(vEdge == cocos2d::ui::LayoutComponent::VerticalEdge::Top) {
                lc->setTopMargin(vMargin);
            } else if(vEdge ==
                      cocos2d::ui::LayoutComponent::VerticalEdge::Bottom) {
                lc->setBottomMargin(vMargin);
            }
        }
        return lc;
    }

    inline cocos2d::ui::LayoutComponent *
    bindDock(cocos2d::Node *node,
             cocos2d::ui::LayoutComponent::HorizontalEdge hEdge, float hMargin,
             cocos2d::ui::LayoutComponent::VerticalEdge vEdge,
             float vMargin = 0.f) {
        auto *lc = cocos2d::ui::LayoutComponent::bindLayoutComponent(node);
        if(hEdge == cocos2d::ui::LayoutComponent::HorizontalEdge::Center) {
            lc->setHorizontalEdge(
                cocos2d::ui::LayoutComponent::HorizontalEdge::None);
            lc->setPositionPercentXEnabled(true);
            lc->setPositionPercentX(0.5f);
        } else {
            lc->setHorizontalEdge(hEdge);
            if(hEdge == cocos2d::ui::LayoutComponent::HorizontalEdge::Left) {
                lc->setLeftMargin(hMargin);
            } else if(hEdge ==
                      cocos2d::ui::LayoutComponent::HorizontalEdge::Right) {
                lc->setRightMargin(hMargin);
            }
        }
        if(vEdge == cocos2d::ui::LayoutComponent::VerticalEdge::Center) {
            lc->setVerticalEdge(
                cocos2d::ui::LayoutComponent::VerticalEdge::None);
            lc->setPositionPercentYEnabled(true);
            lc->setPositionPercentY(0.5f);
        } else {
            lc->setVerticalEdge(vEdge);
            if(vEdge == cocos2d::ui::LayoutComponent::VerticalEdge::Top) {
                lc->setTopMargin(vMargin);
            } else if(vEdge ==
                      cocos2d::ui::LayoutComponent::VerticalEdge::Bottom) {
                lc->setBottomMargin(vMargin);
            }
        }
        return lc;
    }

    inline cocos2d::ui::Layout *
    createRightChevron(const LayoutCtx &ctx, const char *name, float side) {
        auto *dirIcon = cocos2d::ui::Layout::create();
        dirIcon->setName(name);
        dirIcon->setContentSize(cocos2d::Size(side, side));

        const float scaleRatio = side / 80.f;
        const cocos2d::Size armSize(45.f * scaleRatio,
                                    std::max(2.f, 10.f * scaleRatio));
        const cocos2d::Vec2 tipPos(70.f * scaleRatio, side * 0.5f);

        auto *arrow1 = cocos2d::ui::Layout::create();
        arrow1->setContentSize(armSize);
        arrow1->setBackGroundColorType(
            cocos2d::ui::Layout::BackGroundColorType::SOLID);
        arrow1->setBackGroundColor(cocos2d::Color3B(191, 191, 191));
        arrow1->setAnchorPoint(cocos2d::Vec2(0.f, 0.f));
        arrow1->setPosition(tipPos);
        arrow1->setRotation(-135.f);
        dirIcon->addChild(arrow1);

        auto *arrow2 = cocos2d::ui::Layout::create();
        arrow2->setContentSize(armSize);
        arrow2->setBackGroundColorType(
            cocos2d::ui::Layout::BackGroundColorType::SOLID);
        arrow2->setBackGroundColor(cocos2d::Color3B(191, 191, 191));
        arrow2->setAnchorPoint(cocos2d::Vec2(0.f, 1.f));
        arrow2->setPosition(tipPos);
        arrow2->setRotation(135.f);
        dirIcon->addChild(arrow2);

        return dirIcon;
    }

} // namespace Csd
