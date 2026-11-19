#include "ncbind.hpp"

#include <algorithm>
#include <vector>

#define NCB_MODULE_NAME TJS_W("TextRender.dll")

// #define dbg_print TVPAddLog
#define dbg_print(...) ((void)0)

// use Kirikiri-Z rasterizer for layouting
#include "FontRasterizer.h"
#include "LayerBitmapIntf.h"

#include "log/TVPLog.h"

extern FontRasterizer *GetCurrentRasterizer();

using RgbColor = uint32_t;

#define setprop_t(d, p, ty)                                                    \
    {                                                                          \
        tTJSVariant v(ty(p));                                                  \
        d->PropSet(TJS_MEMBERENSURE, TJS_W(#p), nullptr, &v, d);               \
    }

#define setprop(d, p) setprop_t(d, p, )

static bool getDictInt(iTJSDispatch2 *dict, const tjs_char *name, int &out) {
    tTJSVariant v;
    if(TJS_SUCCEEDED(dict->PropGet(0, name, nullptr, &v, dict)) &&
       v.Type() != tvtVoid) {
        out = static_cast<tjs_int>(v);
        return true;
    }
    return false;
}

static bool getDictBool(iTJSDispatch2 *dict, const tjs_char *name, bool &out) {
    int tmp = 0;
    if(getDictInt(dict, name, tmp)) {
        out = (tmp != 0);
        return true;
    }
    return false;
}

static bool getDictReal(iTJSDispatch2 *dict, const tjs_char *name,
                        double &out) {
    tTJSVariant v;
    if(TJS_SUCCEEDED(dict->PropGet(0, name, nullptr, &v, dict)) &&
       v.Type() != tvtVoid) {
        out = static_cast<tTVReal>(v);
        return true;
    }
    return false;
}

static bool getDictColor(iTJSDispatch2 *dict, const tjs_char *name,
                         RgbColor &out) {
    int tmp = 0;
    if(getDictInt(dict, name, tmp)) {
        out = static_cast<RgbColor>(tmp);
        return true;
    }
    return false;
}

static bool getDictString(iTJSDispatch2 *dict, const tjs_char *name,
                          ttstr &out) {
    tTJSVariant v;
    if(TJS_SUCCEEDED(dict->PropGet(0, name, nullptr, &v, dict)) &&
       v.Type() != tvtVoid) {
        out = ttstr(v);
        return true;
    }
    return false;
}

enum TextRenderAlignment {
    kTextRenderAlignmentLeft = -1,
    kTextRenderAlignmentCenter = 0,
    kTextRenderAlignmentRight = 1,
};

struct RubyInfo {
    ttstr text;
    int x = 0;
    int y = 0;
    int cw = 0;
    int size = 0;

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();
        setprop(dict, text);
        setprop(dict, x);
        setprop(dict, y);
        setprop(dict, cw);
        setprop(dict, size);
        tTJSVariant res(dict, dict);
        dict->Release();
        return res;
    }
};

struct KeyWaitInfo {
    int pos = 0;
    int time = 0;

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();
        setprop(dict, pos);
        setprop(dict, time);
        tTJSVariant res(dict, dict);
        dict->Release();
        return res;
    }
};

struct TextRenderState {
    bool bold = false;
    bool italic = false;
    ttstr face = TJS_W("user");
    int fontSize = 24;
    int bigFontSize = 32;
    int smallFontSize = 14;
    double fontScale = 1.0;
    RgbColor chColor = 0xffffff;
    int rubySize = 10;
    int rubyOffset = -2;
    bool shadow = true;
    RgbColor shadowColor = 0x000000;
    int shadowDiff = 0;
    bool edge = false;
    RgbColor edgeColor = 0x0080ff;
    int lineSpacing = 6;
    int pitch = 0;
    int lineSize = 0;
    int align = kTextRenderAlignmentLeft;
    int valign = kTextRenderAlignmentRight;
    bool wordBreak = true;

    bool renderOver = false;
    int renderDelay = 1000;
    ttstr renderText = TJS_W("");

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, bold);
        setprop(dict, italic);
        setprop(dict, fontSize);
        setprop(dict, bigFontSize);
        setprop(dict, smallFontSize);
        setprop(dict, fontScale);
        setprop(dict, face);
        setprop_t(dict, chColor, static_cast<tjs_int>);
        setprop(dict, rubySize);
        setprop(dict, rubyOffset);
        setprop(dict, shadow);
        setprop_t(dict, shadowColor, static_cast<tjs_int>);
        setprop(dict, shadowDiff);
        setprop(dict, edge);
        setprop_t(dict, edgeColor, static_cast<tjs_int>);
        setprop(dict, lineSpacing);
        setprop(dict, pitch);
        setprop(dict, lineSize);
        setprop(dict, align);
        setprop(dict, valign);
        setprop(dict, wordBreak);

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserializeFont(iTJSDispatch2 *dict) {
        if(!dict) {
            return;
        }
        getDictBool(dict, TJS_W("bold"), bold);
        getDictBool(dict, TJS_W("italic"), italic);
        if(!getDictInt(dict, TJS_W("fontSize"), fontSize)) {
            getDictInt(dict, TJS_W("fontsize"), fontSize);
        }
        if(!getDictInt(dict, TJS_W("bigFontSize"), bigFontSize)) {
            getDictInt(dict, TJS_W("bigfontsize"), bigFontSize);
        }
        if(!getDictInt(dict, TJS_W("smallFontSize"), smallFontSize)) {
            getDictInt(dict, TJS_W("smallfontsize"), smallFontSize);
        }
        if(!getDictReal(dict, TJS_W("fontScale"), fontScale)) {
            getDictReal(dict, TJS_W("fontscale"), fontScale);
        }
        getDictString(dict, TJS_W("face"), face);
        if(!getDictColor(dict, TJS_W("chColor"), chColor) &&
           !getDictColor(dict, TJS_W("chcolor"), chColor)) {
            getDictColor(dict, TJS_W("color"), chColor);
        }
        if(!getDictInt(dict, TJS_W("rubySize"), rubySize)) {
            getDictInt(dict, TJS_W("rubysize"), rubySize);
        }
        if(!getDictInt(dict, TJS_W("rubyOffset"), rubyOffset)) {
            getDictInt(dict, TJS_W("rubyoffset"), rubyOffset);
        }
        getDictBool(dict, TJS_W("shadow"), shadow);
        if(!getDictColor(dict, TJS_W("shadowColor"), shadowColor)) {
            getDictColor(dict, TJS_W("shadowcolor"), shadowColor);
        }
        if(!getDictInt(dict, TJS_W("shadowDiff"), shadowDiff)) {
            getDictInt(dict, TJS_W("shadowdiff"), shadowDiff);
        }
        getDictBool(dict, TJS_W("edge"), edge);
        if(!getDictColor(dict, TJS_W("edgeColor"), edgeColor)) {
            getDictColor(dict, TJS_W("edgecolor"), edgeColor);
        }
    }

    void deserializeStyle(iTJSDispatch2 *dict) {
        if(!dict) {
            return;
        }
        if(!getDictInt(dict, TJS_W("lineSpacing"), lineSpacing)) {
            getDictInt(dict, TJS_W("linespacing"), lineSpacing);
        }
        getDictInt(dict, TJS_W("pitch"), pitch);
        if(!getDictInt(dict, TJS_W("lineSize"), lineSize)) {
            getDictInt(dict, TJS_W("linesize"), lineSize);
        }
        getDictInt(dict, TJS_W("align"), align);
        getDictInt(dict, TJS_W("valign"), valign);
        if(!getDictBool(dict, TJS_W("wordBreak"), wordBreak) &&
           !getDictBool(dict, TJS_W("wordbreak"), wordBreak)) {
            getDictBool(dict, TJS_W("word_break"), wordBreak);
        }
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }
        deserializeFont(dict);
        deserializeStyle(dict);
    }

    static TextRenderState from(tTJSVariant t) {
        TextRenderState state{};
        state.deserialize(t);
        return state;
    }
};

struct TextRenderOptions {
    ttstr following = TJS_W(
        "%),:;]}｡｣ﾞﾟ。，、．：；゛゜ヽヾゝゞ々’”）〕］｝〉》」』】°′″℃￠％‰　!."
        "?､･ｧｨｩｪｫｬｭｮｯｰ・？！ーぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ");
    ttstr leading = TJS_W("\\$([{｢‘“（〔［｛〈《「『【￥＄￡");
    ttstr begin = TJS_W("「『（‘“〔［｛〈《");
    ttstr end = TJS_W("」』）’”〕］｝〉》");

    int kinsokuMax = 2;
    bool wordBreak = true;
    bool widthTimeScale = false;
    bool ignoreColor = false;
    bool ignoreSize = false;
    bool ignoreDelay = false;
    bool ignoreOver = false;
    bool ignoreOverY = false;
    bool ignoreOverX = false;
    bool ignoreRuby = false;
    bool ignoreType = false;
    bool ignoreFace = false;
    bool ignoreStyle = false;
    bool ignoreXR = false;

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, following);
        setprop(dict, leading);
        setprop(dict, begin);
        setprop(dict, end);

        tTJSVariant vKinsoku(kinsokuMax);
        dict->PropSet(TJS_MEMBERENSURE, TJS_W("kinsoku_max"), nullptr,
                      &vKinsoku, dict);
        tTJSVariant vWordBreak(wordBreak ? 1 : 0);
        dict->PropSet(TJS_MEMBERENSURE, TJS_W("word_break"), nullptr,
                      &vWordBreak, dict);

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }
        getDictString(dict, TJS_W("following"), following);
        getDictString(dict, TJS_W("leading"), leading);
        getDictString(dict, TJS_W("begin"), begin);
        getDictString(dict, TJS_W("end"), end);
        getDictInt(dict, TJS_W("kinsoku_max"), kinsokuMax);
        if(!getDictBool(dict, TJS_W("word_break"), wordBreak)) {
            getDictBool(dict, TJS_W("wordbreak"), wordBreak);
        }
        getDictBool(dict, TJS_W("width_time_scale"), widthTimeScale);
        getDictBool(dict, TJS_W("ignore_color"), ignoreColor);
        getDictBool(dict, TJS_W("ignore_size"), ignoreSize);
        getDictBool(dict, TJS_W("ignore_delay"), ignoreDelay);
        getDictBool(dict, TJS_W("ignore_over"), ignoreOver);
        getDictBool(dict, TJS_W("ignore_overy"), ignoreOverY);
        getDictBool(dict, TJS_W("ignore_overx"), ignoreOverX);
        getDictBool(dict, TJS_W("ignore_ruby"), ignoreRuby);
        getDictBool(dict, TJS_W("ignore_type"), ignoreType);
        getDictBool(dict, TJS_W("ignore_face"), ignoreFace);
        getDictBool(dict, TJS_W("ignore_style"), ignoreStyle);
        getDictBool(dict, TJS_W("ignore_xr"), ignoreXR);
    }
};

struct CharacterInfo {
    bool bold = false;
    bool italic = false;
    bool graph = false;
    bool vertical = false;
    ttstr face = TJS_W("user");

    int x = 0;
    int y = 0;
    int cw = 0;
    int size = 0;
    int pitch = 0;

    RgbColor color = 0xffffff;
    bool edge = false;
    RgbColor edgeColor = 0x0080ff;
    bool shadow = true;
    RgbColor shadowColor = 0x000000;
    int shadowDiff = 0;

    int delay = 0;
    ttstr link = TJS_W("");
    ttstr linkName = TJS_W("");
    ttstr text = TJS_W("");
    std::vector<RubyInfo> ruby{};

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, bold);
        setprop(dict, italic);
        setprop(dict, graph);
        setprop(dict, vertical);
        setprop(dict, x);
        setprop(dict, y);
        setprop(dict, cw);
        setprop(dict, size);
        setprop(dict, face);

        setprop_t(dict, color, static_cast<tjs_int>);
        if(edge) {
            tTJSVariant vEdge(1);
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("edge"), nullptr, &vEdge,
                          dict);
        } else {
            tTJSVariant vEdge;
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("edge"), nullptr, &vEdge,
                          dict);
        }
        setprop_t(dict, edgeColor, static_cast<tjs_int>);
        if(shadow) {
            tTJSVariant vShadow(1);
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("shadow"), nullptr, &vShadow,
                          dict);
        } else {
            tTJSVariant vShadow;
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("shadow"), nullptr, &vShadow,
                          dict);
        }
        setprop_t(dict, shadowColor, static_cast<tjs_int>);
        setprop(dict, shadowDiff);

        setprop(dict, delay);
        setprop(dict, link);
        setprop(dict, linkName);
        setprop(dict, text);

        if(!ruby.empty()) {
            auto rubyArr = TJSCreateArrayObject();
            for(size_t i = 0; i < ruby.size(); ++i) {
                tTJSVariant rv = ruby[i].serialize();
                rubyArr->PropSetByNum(TJS_MEMBERENSURE, static_cast<tjs_int>(i),
                                      &rv, rubyArr);
            }
            tTJSVariant rubyVar(rubyArr, rubyArr);
            rubyArr->Release();
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("ruby"), nullptr, &rubyVar,
                          dict);
        }

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }

        getDictBool(dict, TJS_W("bold"), bold);
        getDictBool(dict, TJS_W("italic"), italic);
        getDictBool(dict, TJS_W("graph"), graph);
        getDictBool(dict, TJS_W("vertical"), vertical);
        getDictInt(dict, TJS_W("x"), x);
        getDictInt(dict, TJS_W("y"), y);
        getDictInt(dict, TJS_W("cw"), cw);
        getDictInt(dict, TJS_W("size"), size);
        getDictString(dict, TJS_W("face"), face);

        getDictColor(dict, TJS_W("color"), color);
        getDictBool(dict, TJS_W("edge"), edge);
        getDictColor(dict, TJS_W("edgeColor"), edgeColor);
        getDictBool(dict, TJS_W("shadow"), shadow);
        getDictColor(dict, TJS_W("shadowColor"), shadowColor);
        getDictInt(dict, TJS_W("shadowDiff"), shadowDiff);

        getDictInt(dict, TJS_W("delay"), delay);
        getDictString(dict, TJS_W("link"), link);
        getDictString(dict, TJS_W("linkName"), linkName);
        getDictString(dict, TJS_W("text"), text);
    }
};

#define property_accessor(name, type, storage)                                 \
    type get_##name() const { return storage; }                                \
    void set_##name(type v) { storage = v; }

#define property_accessor_cast(name, type, cast, storage)                      \
    cast get_##name() const { return cast(storage); }                          \
    void set_##name(cast v) { storage = type(v); }

#define property_accessor_string(name, storage)                                \
    tTJSVariant get_##name() const { return tTJSVariant(storage); }            \
    void set_##name(tTJSVariant v) { storage = ttstr(v); }

#define property_delegate(name) NCB_PROPERTY(name, get_##name, set_##name);

class TextRenderBase {
public:
    explicit TextRenderBase(iTJSDispatch2 *self = nullptr);
    virtual ~TextRenderBase();

    static tjs_error factory(TextRenderBase **result, tjs_int /*numparams*/,
                             tTJSVariant ** /*param*/, iTJSDispatch2 *objthis) {
        *result = new TextRenderBase(objthis);
        return TJS_S_OK;
    }

    bool render(tTJSString text, int autoIndent = 0, int diff = 0, int all = 0,
                bool same = false, int _reserved = 0);
    static tjs_error rawRender(tTJSVariant *result, tjs_int numparams,
                               tTJSVariant **param, TextRenderBase *self);

    void setRenderSize(int width, int height);
    void setDefault(tTJSVariant defaultSettings);
    void setFont(tTJSVariant fontSettings);
    void setStyle(tTJSVariant styleSettings);
    void setOption(tTJSVariant options);

    tTJSVariant getCharacters(int start = 0, int count = 0);
    static tjs_error rawGetCharacters(tTJSVariant *result, tjs_int numparams,
                                      tTJSVariant **param,
                                      TextRenderBase *self);

    void clear();
    void done();
    void newline();

    void resetFont();
    void resetStyle();
    tTJSVariant getKeyWait();
    void setKeyWait(tTJSVariant) {
        TVPThrowExceptionMessage(TJS_W("no support setKeyWait"));
    }
    int calcShowCount(int elapsed = 0);
    static tjs_error rawCalcShowCount(tTJSVariant *result, tjs_int numparams,
                                      tTJSVariant **param,
                                      TextRenderBase *self);

    tTJSVariant getLinkNames();
    tTJSVariant getLinkCharacters(tTJSVariant link);
    tTJSVariant getLinkRects(tTJSVariant link);
    bool isLinkContains(tTJSVariant link, int x, int y);
    tTJSVariant getLinkOfPosition(int x, int y);
    tTJSVariant findLink(int idx);
    bool contains(int x, int y) const;
    int calcLineOffset(int line) const;

    // property accessor
    property_accessor(vertical, bool, m_vertical);

    bool get_bold() const { return m_state.bold; }
    void set_bold(bool v) {
        m_state.bold = v;
        updateFont();
    }
    bool get_italic() const { return m_state.italic; }
    void set_italic(bool v) {
        m_state.italic = v;
        updateFont();
    }
    tTJSVariant get_face() const { return tTJSVariant(m_state.face); }
    void set_face(tTJSVariant v) {
        m_state.face = ttstr(v);
        updateFont();
    }
    int get_fontSize() const { return m_state.fontSize; }
    void set_fontSize(int v) {
        m_state.fontSize = v;
        updateFont();
    }
    property_accessor(bigFontSize, int, m_state.bigFontSize);
    property_accessor(smallFontSize, int, m_state.smallFontSize);
    double get_fontScale() const { return m_state.fontScale; }
    void set_fontScale(double v) {
        m_state.fontScale = v;
        updateFont();
    }
    property_accessor(timeScale, double, m_timeScale);
    int get_maxScrollOffset() const {
        return std::max(
            0,
            (m_vertical ? (m_renderRight - m_renderLeft) : m_renderBottom) -
                (m_vertical ? m_boxWidth : m_boxHeight));
    }
    void set_maxScrollOffset(int) {}
    int get_maxScrollLine() const { return get_renderLines(); }
    void set_maxScrollLine(int) {}
    property_accessor_cast(chColor, RgbColor, tjs_int, m_state.chColor);
    property_accessor(rubySize, int, m_state.rubySize);
    property_accessor(rubyOffset, int, m_state.rubyOffset);
    property_accessor(shadow, bool, m_state.shadow);
    property_accessor_cast(shadowColor, RgbColor, tjs_int, m_state.shadowColor);
    property_accessor(shadowDiff, int, m_state.shadowDiff);
    property_accessor(edge, bool, m_state.edge);
    property_accessor_cast(edgeColor, RgbColor, tjs_int, m_state.edgeColor);
    property_accessor(lineSpacing, int, m_state.lineSpacing);
    property_accessor(pitch, int, m_state.pitch);
    property_accessor(lineSize, int, m_state.lineSize);
    property_accessor(align, int, m_state.align);
    property_accessor(valign, int, m_state.valign);
    property_accessor(wordBreak, bool, m_state.wordBreak);

    property_accessor(renderOver, bool, m_state.renderOver);
    property_accessor(renderDelay, int, m_state.renderDelay);
    property_accessor(renderLeft, int, m_renderLeft);
    property_accessor(renderRight, int, m_renderRight);
    property_accessor(renderTop, int, m_renderTop);
    property_accessor(renderBottom, int, m_renderBottom);
    property_accessor(renderText, ttstr, m_state.renderText);
    int get_renderCount() const {
        return static_cast<int>(m_characters.size() + m_currentLine.size());
    }
    void set_renderCount(int) {
        TVPThrowExceptionMessage(TJS_W("avoid to set renderCount"));
    }
    int get_renderLines() const {
        return m_renderLines + (m_currentLine.empty() ? 0 : 1);
    }
    void set_renderLines(int) {
        TVPThrowExceptionMessage(TJS_W("avoid to set renderLines"));
    }

    bool get_defaultBold() const { return m_default.bold; }
    void set_defaultBold(bool v) {
        m_default.bold = v;
        m_state.bold = v;
    }
    bool get_defaultItalic() const { return m_default.italic; }
    void set_defaultItalic(bool v) {
        m_default.italic = v;
        m_state.italic = v;
    }
    tTJSVariant get_defaultFace() const { return tTJSVariant(m_default.face); }
    void set_defaultFace(tTJSVariant v) {
        m_default.face = ttstr(v);
        m_state.face = m_default.face;
    }
    int get_defaultFontSize() const { return m_default.fontSize; }
    void set_defaultFontSize(int v) {
        m_default.fontSize = v;
        m_state.fontSize = v;
    }
    int get_defaultBigFontSize() const { return m_default.bigFontSize; }
    void set_defaultBigFontSize(int v) {
        m_default.bigFontSize = v;
        m_state.bigFontSize = v;
    }
    int get_defaultSmallFontSize() const { return m_default.smallFontSize; }
    void set_defaultSmallFontSize(int v) {
        m_default.smallFontSize = v;
        m_state.smallFontSize = v;
    }
    double get_defaultFontScale() const { return m_default.fontScale; }
    void set_defaultFontScale(double v) {
        m_default.fontScale = v;
        m_state.fontScale = v;
    }
    tjs_int get_defaultChColor() const {
        return static_cast<tjs_int>(m_default.chColor);
    }
    void set_defaultChColor(tjs_int v) {
        m_default.chColor = static_cast<RgbColor>(v);
        m_state.chColor = m_default.chColor;
    }
    int get_defaultRubySize() const { return m_default.rubySize; }
    void set_defaultRubySize(int v) {
        m_default.rubySize = v;
        m_state.rubySize = v;
    }
    int get_defaultRubyOffset() const { return m_default.rubyOffset; }
    void set_defaultRubyOffset(int v) {
        m_default.rubyOffset = v;
        m_state.rubyOffset = v;
    }
    bool get_defaultShadow() const { return m_default.shadow; }
    void set_defaultShadow(bool v) {
        m_default.shadow = v;
        m_state.shadow = v;
    }
    tjs_int get_defaultShadowColor() const {
        return static_cast<tjs_int>(m_default.shadowColor);
    }
    void set_defaultShadowColor(tjs_int v) {
        m_default.shadowColor = static_cast<RgbColor>(v);
        m_state.shadowColor = m_default.shadowColor;
    }
    int get_defaultShadowDiff() const { return m_default.shadowDiff; }
    void set_defaultShadowDiff(int v) {
        m_default.shadowDiff = v;
        m_state.shadowDiff = v;
    }
    bool get_defaultEdge() const { return m_default.edge; }
    void set_defaultEdge(bool v) {
        m_default.edge = v;
        m_state.edge = v;
    }
    tjs_int get_defaultEdgeColor() const {
        return static_cast<tjs_int>(m_default.edgeColor);
    }
    void set_defaultEdgeColor(tjs_int v) {
        m_default.edgeColor = static_cast<RgbColor>(v);
        m_state.edgeColor = m_default.edgeColor;
    }
    int get_defaultLineSpacing() const { return m_default.lineSpacing; }
    void set_defaultLineSpacing(int v) {
        m_default.lineSpacing = v;
        m_state.lineSpacing = v;
    }
    int get_defaultPitch() const { return m_default.pitch; }
    void set_defaultPitch(int v) {
        m_default.pitch = v;
        m_state.pitch = v;
    }
    int get_defaultLineSize() const { return m_default.lineSize; }
    void set_defaultLineSize(int v) {
        m_default.lineSize = v;
        m_state.lineSize = v;
    }
    int get_defaultAlign() const { return m_default.align; }
    void set_defaultAlign(int v) {
        m_default.align = v;
        m_state.align = v;
    }
    int get_defaultVAlign() const { return m_default.valign; }
    void set_defaultVAlign(int v) {
        m_default.valign = v;
        m_state.valign = v;
    }
    int get_defaultValign() const { return m_default.valign; }
    void set_defaultValign(int v) {
        m_default.valign = v;
        m_state.valign = v;
    }
    bool get_defaultWordBreak() const { return m_default.wordBreak; }
    void set_defaultWordBreak(bool v) {
        m_default.wordBreak = v;
        m_state.wordBreak = v;
        m_options.wordBreak = v;
    }

private:
    iTJSDispatch2 *m_self = nullptr;
    bool m_inUpdateFont = false;
    double m_timeScale = 1.0;

    int m_boxWidth = 0;
    int m_boxHeight = 0;

    int m_x = 0;
    int m_y = 0;

    int m_renderLeft = 0;
    int m_renderTop = 0;
    int m_renderRight = 0;
    int m_renderBottom = 0;
    int m_renderLines = 0;

    int m_indent = 0;
    int m_autoIndent = 0;
    tjs_char m_autoIndentEndChar = 0;
    bool m_overflow = false;
    bool m_isBeginningOfLine = true;

    bool m_vertical = false;

    int m_baseChSpeed = 0;
    int m_currentChSpeed = 0;
    int m_delayAccum = 0;
    ttstr m_currentLink = TJS_W("");
    ttstr m_currentLinkName = TJS_W("");
    ttstr m_pendingRuby = TJS_W("");
    int m_pendingRubyCount = 0;

    TextRenderOptions m_options{};
    TextRenderState m_default{};
    TextRenderState m_state{};

    std::vector<CharacterInfo> m_characters{};
    std::vector<CharacterInfo> m_currentLine{};
    std::vector<KeyWaitInfo> m_keyWait{};

    // State for word_break == false (Western word-wrapping)
    bool m_prevWasStick = false;
    int m_wordStartIdx = -1;

    // State for word_break == true (CJK kinsoku)
    int m_followingHangCount = 0;

    int getEffectiveSize(int baseSize) const;
    int getEffectiveFontSize() const;
    int getEffectivePitch() const;
    int getEffectiveLineSpacing() const;
    int getEffectiveLineSize() const;

    int measureTextWidth(ttstr const &text, int pixelSize);
    void placeCharacterInfo(CharacterInfo info);
    void commitCurrentLine(bool advanceLine);
    void updateBoundsAndOverflow();
    void attachPendingRuby(size_t targetIndex, size_t targetCount,
                           ttstr const &rubyText);

    void pushCharacter(tjs_char ch);
    void pushGraphicalCharacter(ttstr const &graph);
    void performLinebreak();
    void flush(bool force = false);
    void updateFont();
};

// -------------------------------------------------------------------

TextRenderBase::TextRenderBase(iTJSDispatch2 *self) : m_self(self) {}

TextRenderBase::~TextRenderBase() {}

int TextRenderBase::getEffectiveSize(int baseSize) const {
    int sz = static_cast<int>(baseSize * m_state.fontScale);
    return sz > 0 ? sz : 1;
}

int TextRenderBase::getEffectiveFontSize() const {
    return getEffectiveSize(m_state.fontSize);
}

int TextRenderBase::getEffectivePitch() const {
    return static_cast<int>(m_state.pitch * m_state.fontScale);
}

int TextRenderBase::getEffectiveLineSpacing() const {
    return static_cast<int>(m_state.lineSpacing * m_state.fontScale);
}

int TextRenderBase::getEffectiveLineSize() const {
    return static_cast<int>(m_state.lineSize * m_state.fontScale);
}

static bool readchar(tTJSString const &str, size_t &i, tjs_char &c) {
    auto const len = static_cast<size_t>(str.GetLen());

    if(++i >= len) {
        return false;
    }

    c = str[static_cast<tjs_int>(i)];
    return true;
}

static ttstr read_until_semicolon(tTJSString const &str, size_t &i) {
    ttstr out{};
    tjs_char ch;
    while(readchar(str, i, ch)) {
        if(ch == ';') {
            break;
        }
        out += ch;
    }
    return out;
}

static bool read_optional_integer(tTJSString const &str, size_t &i,
                                  int &value) {
    auto const len = static_cast<size_t>(str.GetLen());
    bool is_negative = false;
    bool has_digits = false;
    while(i + 1 < len) {
        tjs_char ch = str[static_cast<tjs_int>(i + 1)];
        if(ch == ';') {
            ++i;
            break;
        }
        if('0' <= ch && ch <= '9') {
            ++i;
            value = value * 10 + (ch - '0');
            has_digits = true;
            continue;
        }
        if(ch == '-' && !has_digits) {
            ++i;
            is_negative = !is_negative;
            continue;
        }
        break;
    }
    if(is_negative) {
        value = -value;
    }
    return has_digits;
}

static bool read_optional_hex_color(tTJSString const &str, size_t &i,
                                    RgbColor &colour) {
    colour = 0;
    bool has_hex = false;
    tjs_char ch;
    while(readchar(str, i, ch)) {
        if(ch == ';') {
            break;
        }
        RgbColor c = 0;
        if('0' <= ch && ch <= '9') {
            c = static_cast<RgbColor>(ch - '0');
        } else if('A' <= ch && ch <= 'F') {
            c = 0x0a + static_cast<RgbColor>(ch - 'A');
        } else if('a' <= ch && ch <= 'f') {
            c = 0x0a + static_cast<RgbColor>(ch - 'a');
        } else {
            continue;
        }
        has_hex = true;
        colour = (colour << 4) | c;
    }
    return has_hex;
}

tjs_error TextRenderBase::rawRender(tTJSVariant *result, tjs_int numparams,
                                    tTJSVariant **param, TextRenderBase *self) {
    if(numparams < 1) {
        return TJS_E_BADPARAMCOUNT;
    }
    tTJSString text = *param[0];
    int autoIndent = (numparams >= 2 && param[1]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[1])
        : 0;
    int diff = (numparams >= 3 && param[2]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[2])
        : 0;
    int all = (numparams >= 4 && param[3]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[3])
        : 0;
    bool same = (numparams >= 5 && param[4]->Type() != tvtVoid)
        ? (static_cast<tjs_int>(*param[4]) != 0)
        : false;
    int reserved = (numparams >= 6 && param[5]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[5])
        : 0;

    bool ok = self->render(text, autoIndent, diff, all, same, reserved);
    if(result) {
        *result = ok ? 1 : 0;
    }
    return TJS_S_OK;
}

bool TextRenderBase::render(tTJSString text, int autoIndent, int diff, int all,
                            bool same, int /*_reserved*/) {
    m_autoIndent = autoIndent;
    auto const len = static_cast<size_t>(text.GetLen());

    if(m_options.ignoreDelay || (diff <= 0 && all <= 0)) {
        m_baseChSpeed = 0;
        m_currentChSpeed = 0;
        m_delayAccum = 0;
        m_state.renderDelay = 0;
    } else {
        m_baseChSpeed = diff > 0
            ? diff
            : (len > 0 ? std::max(1, static_cast<int>(all / len)) : 0);
        m_currentChSpeed = m_baseChSpeed;
        if(same) {
            m_delayAccum = 0;
        }
    }

    updateFont();

    auto evalLabelValue = [&](ttstr const &labelName,
                              int fallbackValue) -> int {
        if(!labelName.IsEmpty() && m_self) {
            tTJSVariant arg(labelName);
            tTJSVariant *argP = &arg;
            tTJSVariant res;
            if(TJS_SUCCEEDED(m_self->FuncCall(0, TJS_W("onLabel"), nullptr,
                                              &res, 1, &argP, m_self)) &&
               res.Type() != tvtVoid) {
                return static_cast<tjs_int>(res);
            }
        }
        return fallbackValue;
    };

    for(size_t i = 0; i < len; ++i) {
        auto ch = text[static_cast<tjs_int>(i)];

        switch(ch) {
            case '\n':
                performLinebreak();
                break;
            case '%':
                if(!readchar(text, i, ch)) {
                    break;
                }

                switch(ch) {
                    case ';':
                        if(!m_options.ignoreSize) {
                            m_state.fontSize = m_default.fontSize;
                            updateFont();
                        }
                        break;
                    case 'f': {
                        ttstr faceName = read_until_semicolon(text, i);
                        if(!m_options.ignoreFace) {
                            m_state.face =
                                faceName.IsEmpty() ? m_default.face : faceName;
                            updateFont();
                        }
                        break;
                    }
                    case 'b': {
                        if(!readchar(text, i, ch)) {
                            break;
                        }
                        if(!m_options.ignoreStyle) {
                            m_state.bold = (ch == '1')
                                ? true
                                : (ch == '0' ? false : m_default.bold);
                            updateFont();
                        }
                        break;
                    }
                    case 'i': {
                        if(!readchar(text, i, ch)) {
                            break;
                        }
                        if(!m_options.ignoreStyle) {
                            m_state.italic = (ch == '1')
                                ? true
                                : (ch == '0' ? false : m_default.italic);
                            updateFont();
                        }
                        break;
                    }
                    case 's': {
                        if(!readchar(text, i, ch)) {
                            break;
                        }
                        if(ch == '#') {
                            RgbColor colour = 0;
                            bool hasHex =
                                read_optional_hex_color(text, i, colour);
                            if(!m_options.ignoreColor) {
                                m_state.shadowColor =
                                    hasHex ? colour : m_default.shadowColor;
                            }
                        } else if(!m_options.ignoreStyle) {
                            m_state.shadow = (ch == '1')
                                ? true
                                : (ch == '0' ? false : m_default.shadow);
                        }
                        break;
                    }
                    case 'e': {
                        if(!readchar(text, i, ch)) {
                            break;
                        }
                        if(ch == '#') {
                            RgbColor colour = 0;
                            bool hasHex =
                                read_optional_hex_color(text, i, colour);
                            if(!m_options.ignoreColor) {
                                m_state.edgeColor =
                                    hasHex ? colour : m_default.edgeColor;
                            }
                        } else if(!m_options.ignoreStyle) {
                            m_state.edge = (ch == '1')
                                ? true
                                : (ch == '0' ? false : m_default.edge);
                        }
                        break;
                    }
                    case 'B':
                        if(!m_options.ignoreSize) {
                            m_state.fontSize = m_default.bigFontSize;
                            updateFont();
                        }
                        break;
                    case 'S':
                        if(!m_options.ignoreSize) {
                            m_state.fontSize = m_default.smallFontSize;
                            updateFont();
                        }
                        break;
                    case 'r':
                        resetFont();
                        break;
                    case 'C':
                        if(!m_options.ignoreStyle) {
                            m_state.align = kTextRenderAlignmentCenter;
                        }
                        break;
                    case 'R':
                        if(!m_options.ignoreStyle) {
                            m_state.align = kTextRenderAlignmentRight;
                        }
                        break;
                    case 'L':
                        if(!m_options.ignoreStyle) {
                            m_state.align = kTextRenderAlignmentLeft;
                        }
                        break;
                    case 'l': {
                        ttstr linkTarget = read_until_semicolon(text, i);
                        m_currentLink = linkTarget;
                        if(linkTarget.IsEmpty()) {
                            m_currentLinkName.Clear();
                        }
                        break;
                    }
                    case 'p': {
                        int value = 0;
                        bool hasDigits = read_optional_integer(text, i, value);
                        if(!m_options.ignoreStyle) {
                            m_state.pitch = hasDigits ? value : m_default.pitch;
                        }
                        break;
                    }
                    case 'n': {
                        int count = 0;
                        bool hasDigits = read_optional_integer(text, i, count);
                        if(!hasDigits) {
                            count = 1;
                        }
                        if(!m_options.ignoreXR) {
                            for(int k = 0; k < count; ++k) {
                                performLinebreak();
                            }
                        }
                        break;
                    }
                    case 'k': {
                        if(!readchar(text, i, ch)) {
                            break;
                        }
                        m_state.wordBreak = (ch == '1')
                            ? true
                            : (ch == '0' ? false : m_options.wordBreak);
                        break;
                    }
                    case 'd': {
                        int value = 0;
                        bool hasDigits = read_optional_integer(text, i, value);
                        if(!m_options.ignoreDelay) {
                            m_currentChSpeed = hasDigits
                                ? (m_baseChSpeed * value / 100)
                                : m_baseChSpeed;
                        }
                        break;
                    }
                    case 'a': {
                        int value = 0;
                        bool hasDigits = read_optional_integer(text, i, value);
                        if(!m_options.ignoreDelay) {
                            m_currentChSpeed =
                                hasDigits ? value : m_baseChSpeed;
                        }
                        break;
                    }
                    case 'w': {
                        if(i + 1 < len &&
                           text[static_cast<tjs_int>(i + 1)] == '$') {
                            ++i;
                            ttstr labelName = read_until_semicolon(text, i);
                            int value = evalLabelValue(labelName, 100);
                            if(!m_options.ignoreDelay && m_baseChSpeed > 0) {
                                m_delayAccum += m_baseChSpeed * value / 100;
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        } else {
                            int value = 0;
                            bool hasDigits =
                                read_optional_integer(text, i, value);
                            if(!hasDigits) {
                                value = 100;
                            }
                            if(!m_options.ignoreDelay && m_baseChSpeed > 0) {
                                m_delayAccum += m_baseChSpeed * value / 100;
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        }
                        break;
                    }
                    case 't': {
                        if(i + 1 < len &&
                           text[static_cast<tjs_int>(i + 1)] == '$') {
                            ++i;
                            ttstr labelName = read_until_semicolon(text, i);
                            int value = evalLabelValue(labelName, 0);
                            if(!m_options.ignoreDelay) {
                                m_delayAccum += value;
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        } else {
                            int value = 0;
                            read_optional_integer(text, i, value);
                            if(!m_options.ignoreDelay) {
                                m_delayAccum += value;
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        }
                        break;
                    }
                    case 'D': {
                        if(i + 1 < len &&
                           text[static_cast<tjs_int>(i + 1)] == '$') {
                            ++i;
                            ttstr labelName = read_until_semicolon(text, i);
                            if(!m_options.ignoreDelay) {
                                m_delayAccum =
                                    evalLabelValue(labelName, m_delayAccum);
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        } else {
                            int value = 0;
                            bool hasDigits =
                                read_optional_integer(text, i, value);
                            if(!m_options.ignoreDelay) {
                                m_delayAccum = hasDigits ? value : 0;
                                m_state.renderDelay =
                                    std::max(m_state.renderDelay, m_delayAccum);
                            }
                        }
                        break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        int value = static_cast<int>(ch - '0');
                        read_optional_integer(text, i, value);
                        if(!m_options.ignoreSize) {
                            m_state.fontSize = m_default.fontSize * value / 100;
                            updateFont();
                        }
                        break;
                    }
                    case '%':
                        pushCharacter(TJS_W('%'));
                        break;
                    default:
                        read_until_semicolon(text, i);
                        break;
                }
                break;
            case '\\':
                if(!readchar(text, i, ch)) {
                    break;
                }

                switch(ch) {
                    case 'n':
                        performLinebreak();
                        break;
                    case 't':
                        pushCharacter(TJS_W('\t'));
                        break;
                    case 'i':
                        m_indent = m_vertical ? m_y : m_x;
                        break;
                    case 'r':
                        if(!m_options.ignoreXR) {
                            m_indent = 0;
                            m_autoIndentEndChar = 0;
                        }
                        break;
                    case 'w': {
                        int halfSpace = std::max(1, getEffectiveFontSize() / 2);
                        if(m_vertical) {
                            m_y += halfSpace;
                        } else {
                            m_x += halfSpace;
                        }
                        m_isBeginningOfLine = false;
                        m_prevWasStick = false;
                        m_wordStartIdx = -1;
                        break;
                    }
                    case 'x':
                        m_isBeginningOfLine = false;
                        m_prevWasStick = false;
                        m_wordStartIdx = -1;
                        break;
                    case 'k':
                        if(!m_options.ignoreDelay && !m_options.ignoreXR) {
                            int pos = static_cast<int>(m_characters.size() +
                                                       m_currentLine.size());
                            m_keyWait.push_back(
                                KeyWaitInfo{ pos, m_delayAccum });
                        }
                        break;
                    default:
                        pushCharacter(ch);
                        break;
                }
                break;
            case '[': {
                ttstr rubySpec{};
                while(readchar(text, i, ch)) {
                    if(ch == ']') {
                        break;
                    }
                    rubySpec += ch;
                }

                if(!m_options.ignoreRuby && !rubySpec.IsEmpty()) {
                    ttstr rubyText = rubySpec;
                    int rubyCharCount = 1;
                    const tjs_char *raw = rubySpec.c_str();
                    tjs_int commaIdx = -1;
                    for(tjs_int k = rubySpec.GetLen() - 1; k >= 0; --k) {
                        if(raw[k] == TJS_W(',')) {
                            commaIdx = k;
                            break;
                        }
                    }
                    if(commaIdx > 0) {
                        int parsedCount = 0;
                        bool allDigits = true;
                        for(tjs_int k = commaIdx + 1; k < rubySpec.GetLen();
                            ++k) {
                            if(raw[k] >= TJS_W('0') && raw[k] <= TJS_W('9')) {
                                parsedCount =
                                    parsedCount * 10 + (raw[k] - TJS_W('0'));
                            } else {
                                allDigits = false;
                                break;
                            }
                        }
                        if(allDigits && parsedCount > 0) {
                            rubyText = tTJSString(raw, commaIdx);
                            rubyCharCount = parsedCount;
                        }
                    }
                    m_pendingRuby = rubyText;
                    m_pendingRubyCount = rubyCharCount;
                }

                break;
            }
            case '#': {
                RgbColor colour = 0;
                bool hasHex = read_optional_hex_color(text, i, colour);
                if(!m_options.ignoreColor) {
                    m_state.chColor = hasHex ? colour : m_default.chColor;
                }
                break;
            }
            case '&': {
                ttstr graph = read_until_semicolon(text, i);
                if(!graph.IsEmpty()) {
                    pushGraphicalCharacter(graph);
                }
                break;
            }
            case '$': {
                ttstr expr = read_until_semicolon(text, i);
                if(m_self && !expr.IsEmpty()) {
                    tTJSVariant arg(expr);
                    tTJSVariant *argP = &arg;
                    tTJSVariant res;
                    if(TJS_SUCCEEDED(m_self->FuncCall(0, TJS_W("onEval"),
                                                      nullptr, &res, 1, &argP,
                                                      m_self)) &&
                       res.Type() != tvtVoid) {
                        ttstr evaluated(res);
                        for(tjs_int k = 0; k < evaluated.GetLen(); ++k) {
                            pushCharacter(evaluated[k]);
                        }
                    }
                }
                break;
            }
            default:
                pushCharacter(ch);
                break;
        }
    }

    updateBoundsAndOverflow();
    return !m_state.renderOver;
}

int TextRenderBase::measureTextWidth(ttstr const &text, int pixelSize) {
    if(text.IsEmpty()) {
        return 0;
    }

    if(m_self) {
        tTJSVariant argText(text);
        tTJSVariant argSize(pixelSize);
        tTJSVariant *args[2] = { &argText, &argSize };
        tTJSVariant res;
        if(TJS_SUCCEEDED(m_self->FuncCall(0, TJS_W("onGetTextWidth"), nullptr,
                                          &res, 2, args, m_self)) &&
           res.Type() != tvtVoid) {
            int w = static_cast<tjs_int>(res);
            if(w > 0) {
                return w;
            }
        }
    }

    if(auto rasterizer = GetCurrentRasterizer()) {
        int totalW = 0;
        for(tjs_int i = 0; i < text.GetLen(); ++i) {
            int w = 0, h = 0;
            rasterizer->GetTextExtent(text[i], w, h);
            totalW += w;
        }
        if(totalW > 0) {
            return totalW;
        }
    }

    int fallback = 0;
    for(tjs_int i = 0; i < text.GetLen(); ++i) {
        fallback += (text[i] < 0x80) ? std::max(1, pixelSize / 2) : pixelSize;
    }
    return fallback;
}

static bool findchInChars(const tjs_char *chz, tjs_char ch) {
    if(!chz || ch == 0) {
        return false;
    }
    for(int i = 0; chz[i] != L'\0'; i++) {
        if(chz[i] == ch) {
            return true;
        }
    }
    return false;
}

static int findCharIndex(const tjs_char *chz, tjs_char ch) {
    if(!chz || ch == 0) {
        return -1;
    }
    for(int i = 0; chz[i] != L'\0'; i++) {
        if(chz[i] == ch) {
            return i;
        }
    }
    return -1;
}

static bool isWordWrapSpace(tjs_char ch) {
    return ch == TJS_W(' ') || ch == TJS_W('\t') || ch == 0x3000;
}

static bool isWordWrapStickChar(tjs_char ch) {
    return ch > 0x20 && !isWordWrapSpace(ch);
}

void TextRenderBase::commitCurrentLine(bool advanceLine) {
    int lineSize = std::max(getEffectiveLineSize(), getEffectiveFontSize());
    for(auto const &ch : m_currentLine) {
        lineSize = std::max(lineSize, m_vertical ? ch.cw : ch.size);
    }

    if(!m_currentLine.empty()) {
        int lineStart =
            m_vertical ? m_currentLine.front().y : m_currentLine.front().x;
        int lineEnd = m_vertical
            ? (m_currentLine.back().y + m_currentLine.back().size)
            : (m_currentLine.back().x + m_currentLine.back().cw);
        int contentExtent = std::max(0, lineEnd - lineStart);
        int boxExtent = m_vertical ? m_boxHeight : m_boxWidth;
        int alignOffset = 0;
        if(boxExtent > 0) {
            if(m_state.align == kTextRenderAlignmentCenter) {
                alignOffset =
                    std::max(0, (boxExtent - lineStart - contentExtent) / 2);
            } else if(m_state.align == kTextRenderAlignmentRight) {
                alignOffset =
                    std::max(0, boxExtent - lineStart - contentExtent);
            }
        }

        for(auto &ch : m_currentLine) {
            if(m_vertical) {
                ch.y += alignOffset;
                if(m_state.valign == kTextRenderAlignmentLeft) {
                    ch.x = m_x - (lineSize - ch.cw);
                } else if(m_state.valign == kTextRenderAlignmentCenter) {
                    ch.x = m_x - (lineSize - ch.cw) / 2;
                } else {
                    ch.x = m_x;
                }
            } else {
                ch.x += alignOffset;
                if(m_state.valign == kTextRenderAlignmentRight) {
                    ch.y = m_y + (lineSize - ch.size);
                } else if(m_state.valign == kTextRenderAlignmentCenter) {
                    ch.y = m_y + (lineSize - ch.size) / 2;
                } else {
                    ch.y = m_y;
                }
            }
            m_state.renderText += ch.text;
        }

        m_characters.insert(m_characters.end(), m_currentLine.begin(),
                            m_currentLine.end());
        m_currentLine.clear();
        ++m_renderLines;
    }

    if(advanceLine) {
        m_state.renderText += TJS_W("\n");
        int step = lineSize + getEffectiveLineSpacing();
        if(m_vertical) {
            m_x -= step;
            m_y = m_indent;
        } else {
            m_y += step;
            m_x = m_indent;
        }
        m_isBeginningOfLine = true;
    }

    m_prevWasStick = false;
    m_wordStartIdx = -1;
    m_followingHangCount = 0;
}

void TextRenderBase::performLinebreak() { commitCurrentLine(true); }

void TextRenderBase::flush(bool /*force*/) {
    if(!m_currentLine.empty()) {
        commitCurrentLine(false);
    }
    updateBoundsAndOverflow();
}

void TextRenderBase::updateBoundsAndOverflow() {
    bool hasAny = false;
    int left = 0, top = 0, right = 0, bottom = 0;

    auto accumulate = [&](CharacterInfo const &ch, int yBase, int lineSz) {
        int cx = ch.x;
        int cy = ch.y;
        if(!m_vertical && yBase >= 0) {
            if(m_state.valign == kTextRenderAlignmentRight) {
                cy = yBase + (lineSz - ch.size);
            } else if(m_state.valign == kTextRenderAlignmentCenter) {
                cy = yBase + (lineSz - ch.size) / 2;
            }
        }
        int cr = cx + ch.cw;
        int cb = cy + ch.size;
        if(!hasAny) {
            left = cx;
            top = cy;
            right = cr;
            bottom = cb;
            hasAny = true;
        } else {
            left = std::min(left, cx);
            top = std::min(top, cy);
            right = std::max(right, cr);
            bottom = std::max(bottom, cb);
        }
    };

    for(auto const &ch : m_characters) {
        accumulate(ch, -1, 0);
    }

    if(!m_currentLine.empty()) {
        int lineSize = std::max(getEffectiveLineSize(), getEffectiveFontSize());
        for(auto const &ch : m_currentLine) {
            lineSize = std::max(lineSize, m_vertical ? ch.cw : ch.size);
        }
        for(auto const &ch : m_currentLine) {
            accumulate(ch, m_y, lineSize);
        }
    }

    m_renderLeft = hasAny ? left : 0;
    m_renderTop = hasAny ? top : 0;
    m_renderRight = hasAny ? right : 0;
    m_renderBottom = hasAny ? bottom : 0;

    bool over = false;
    if(!m_options.ignoreOver && hasAny) {
        if(m_vertical) {
            if(!m_options.ignoreOverX && m_boxWidth > 0 &&
               (m_renderRight - m_renderLeft) > m_boxWidth) {
                over = true;
            }
        } else {
            if(!m_options.ignoreOverY && m_boxHeight > 0 &&
               m_renderBottom > m_boxHeight) {
                over = true;
            }
        }
    }

    m_overflow = over;
    m_state.renderOver = over;
}

void TextRenderBase::placeCharacterInfo(CharacterInfo info) {
    tjs_char ch = (!info.graph && !info.text.IsEmpty())
        ? info.text[0]
        : static_cast<tjs_char>(0);

    bool isSpace = !info.graph && isWordWrapSpace(ch);
    bool isStick = !info.graph && isWordWrapStickChar(ch);
    bool isLeading =
        !info.graph && findchInChars(m_options.leading.c_str(), ch);
    bool isFollowing =
        !info.graph && findchInChars(m_options.following.c_str(), ch);

    int charAdvance = m_vertical ? info.size : info.cw;
    int boxLimit = m_vertical ? m_boxHeight : m_boxWidth;
    bool ignoreOverAxis = m_options.ignoreOver ||
        (m_vertical ? m_options.ignoreOverY : m_options.ignoreOverX);
    int curPos = m_vertical ? m_y : m_x;

    // Auto-indent handling on opening/closing brackets
    if(m_autoIndent != 0 && !info.graph) {
        int beginIdx = findCharIndex(m_options.begin.c_str(), ch);
        int endIdx = findCharIndex(m_options.end.c_str(), ch);
        if(m_characters.empty() && m_currentLine.empty() && beginIdx >= 0) {
            if(m_autoIndent < 0) {
                curPos -= charAdvance;
                if(m_vertical) {
                    m_y = curPos;
                } else {
                    m_x = curPos;
                }
            }
            m_indent = curPos + charAdvance + info.pitch;
            if(beginIdx < m_options.end.GetLen()) {
                m_autoIndentEndChar = m_options.end[beginIdx];
            }
        } else if(endIdx >= 0 && m_indent > 0 &&
                  (m_autoIndentEndChar == 0 || ch == m_autoIndentEndChar)) {
            m_indent = 0;
            m_autoIndentEndChar = 0;
        }
    }

    if(!m_state.wordBreak) {
        // Western word-wrap mode (word_break == 0)
        // Record the start index of a word BEFORE pushing its first character
        // onto the current line, so that if any character of this word
        // overflows, the ENTIRE word (including its first character) moves to
        // the next line.
        if(isStick && !m_prevWasStick && !m_currentLine.empty()) {
            m_wordStartIdx = static_cast<int>(m_currentLine.size());
        }

        bool overflows = !ignoreOverAxis && boxLimit > 0 &&
            !m_currentLine.empty() && (curPos + charAdvance > boxLimit);

        if(overflows) {
            if(isSpace) {
                // Place trailing space at the end of the current line and wrap
                // immediately so the next word starts cleanly at m_indent.
                info.x = m_x;
                info.y = m_y;
                m_currentLine.push_back(std::move(info));
                commitCurrentLine(true);
                return;
            }

            if(isStick && m_wordStartIdx > 0 &&
               m_wordStartIdx <= static_cast<int>(m_currentLine.size())) {
                // Pop the partial word (including its first character at
                // m_wordStartIdx) from the current line, wrap the line, and
                // re-place the entire word on the new line.
                std::vector<CharacterInfo> wordChars(m_currentLine.begin() +
                                                         m_wordStartIdx,
                                                     m_currentLine.end());
                m_currentLine.erase(m_currentLine.begin() + m_wordStartIdx,
                                    m_currentLine.end());

                commitCurrentLine(true);

                for(auto &wc : wordChars) {
                    wc.x = m_x;
                    wc.y = m_y;
                    if(m_vertical) {
                        m_y += wc.size + wc.pitch;
                    } else {
                        m_x += wc.cw + wc.pitch;
                    }
                    m_currentLine.push_back(std::move(wc));
                }
                m_prevWasStick = true;
                m_wordStartIdx = -1;
                m_isBeginningOfLine = false;
                curPos = m_vertical ? m_y : m_x;
            } else {
                // Either the first character of the new word itself overflowed
                // (m_wordStartIdx == static_cast<int>(m_currentLine.size()))
                // or a single word is wider than the entire box (m_wordStartIdx
                // == -1).
                commitCurrentLine(true);
                curPos = m_vertical ? m_y : m_x;
            }
        }

        info.x = m_x;
        info.y = m_y;
        if(m_vertical) {
            m_y += info.size + info.pitch;
        } else {
            m_x += info.cw + info.pitch;
        }
        m_currentLine.push_back(std::move(info));
        m_prevWasStick = isStick;
        if(!isStick) {
            m_wordStartIdx = -1;
        }
        m_isBeginningOfLine = false;
        return;
    }

    // CJK character wrap with kinsoku rules (word_break == 1)
    bool overflows = !ignoreOverAxis && boxLimit > 0 &&
        !m_currentLine.empty() &&
        (curPos + charAdvance > boxLimit || m_followingHangCount > 0);

    if(overflows) {
        if(isFollowing && m_followingHangCount < m_options.kinsokuMax) {
            // Allow up to kinsokuMax following characters to hang at line end
            ++m_followingHangCount;
        } else {
            // Check if trailing characters on m_currentLine must move to the
            // next line (e.g., leading characters like '「' or '（', or a
            // kinsoku cluster when following exceeds kinsokuMax)
            int moveFrom = static_cast<int>(m_currentLine.size());
            if(isFollowing) {
                while(moveFrom > 0) {
                    tjs_char pc = m_currentLine[moveFrom - 1].text.IsEmpty()
                        ? 0
                        : m_currentLine[moveFrom - 1].text[0];
                    if(findchInChars(m_options.following.c_str(), pc)) {
                        --moveFrom;
                    } else {
                        break;
                    }
                }
                if(moveFrom > 0) {
                    --moveFrom;
                }
            }
            while(moveFrom > 0) {
                tjs_char pc = m_currentLine[moveFrom - 1].text.IsEmpty()
                    ? 0
                    : m_currentLine[moveFrom - 1].text[0];
                if(findchInChars(m_options.leading.c_str(), pc)) {
                    --moveFrom;
                } else {
                    break;
                }
            }

            if(moveFrom > 0 &&
               moveFrom < static_cast<int>(m_currentLine.size())) {
                std::vector<CharacterInfo> moved(
                    m_currentLine.begin() + moveFrom, m_currentLine.end());
                m_currentLine.erase(m_currentLine.begin() + moveFrom,
                                    m_currentLine.end());
                commitCurrentLine(true);
                for(auto &mc : moved) {
                    mc.x = m_x;
                    mc.y = m_y;
                    if(m_vertical) {
                        m_y += mc.size + mc.pitch;
                    } else {
                        m_x += mc.cw + mc.pitch;
                    }
                    m_currentLine.push_back(std::move(mc));
                }
            } else {
                commitCurrentLine(true);
            }
        }
    }

    info.x = m_x;
    info.y = m_y;
    if(m_vertical) {
        m_y += info.size + info.pitch;
    } else {
        m_x += info.cw + info.pitch;
    }
    m_currentLine.push_back(std::move(info));
    m_isBeginningOfLine = false;
    (void)isLeading;
}

void TextRenderBase::attachPendingRuby(size_t targetIndex, size_t targetCount,
                                       ttstr const &rubyText) {
    if(rubyText.IsEmpty() || targetIndex >= m_currentLine.size()) {
        return;
    }
    int rSize = getEffectiveSize(m_state.rubySize);
    int rOffset = static_cast<int>(m_state.rubyOffset * m_state.fontScale);
    int rWidth = measureTextWidth(rubyText, rSize);

    size_t endIndex = std::min(m_currentLine.size(),
                               targetIndex + std::max<size_t>(1, targetCount));
    int spanWidth = 0;
    for(size_t i = targetIndex; i < endIndex; ++i) {
        spanWidth += m_currentLine[i].cw;
    }

    RubyInfo ruby{};
    ruby.text = rubyText;
    ruby.size = rSize;
    ruby.cw = rWidth;
    if(m_vertical) {
        ruby.x = m_currentLine[targetIndex].cw - rOffset;
        ruby.y = (spanWidth - rWidth) / 2;
    } else {
        ruby.x = (spanWidth - rWidth) / 2;
        ruby.y = -rSize - rOffset;
    }
    m_currentLine[targetIndex].ruby.push_back(std::move(ruby));
}

void TextRenderBase::pushGraphicalCharacter(ttstr const &graph) {
    int width = getEffectiveFontSize();
    int height = width;

    if(m_self) {
        tTJSVariant arg(graph);
        tTJSVariant *argP = &arg;
        tTJSVariant res;
        if(TJS_SUCCEEDED(m_self->FuncCall(0, TJS_W("onGetGraphSize"), nullptr,
                                          &res, 1, &argP, m_self)) &&
           res.Type() == tvtObject) {
            if(auto dict = res.AsObjectNoAddRef()) {
                getDictInt(dict, TJS_W("width"), width);
                getDictInt(dict, TJS_W("height"), height);
            }
        }
    }

    if(!m_options.ignoreDelay && m_currentChSpeed > 0) {
        m_delayAccum += m_currentChSpeed;
        m_state.renderDelay = std::max(m_state.renderDelay, m_delayAccum);
    }

    CharacterInfo info{};
    info.bold = m_state.bold;
    info.italic = m_state.italic;
    info.graph = true;
    info.vertical = m_vertical;
    info.face = m_state.face;
    info.cw = width;
    info.size = height;
    info.pitch = getEffectivePitch();
    info.color = m_state.chColor;
    info.edge = m_state.edge;
    info.edgeColor = m_state.edgeColor;
    info.shadow = m_state.shadow;
    info.shadowColor = m_state.shadowColor;
    info.shadowDiff = m_state.shadowDiff;
    info.delay = m_delayAccum;
    info.link = m_currentLink;
    info.linkName = m_currentLinkName;
    info.text = graph;

    placeCharacterInfo(std::move(info));
}

void TextRenderBase::pushCharacter(tjs_char ch) {
    if((0xD800 <= ch && ch <= 0xDBFF) /* upper surrogate-pair */
       || (0xDC00 <= ch && ch <= 0xDFFF) /* lower surrogate-pair */) {
        TVPThrowExceptionMessage(TJS_W("unexpected character: surrogate pair"));
    }

    int effectiveFontSize = getEffectiveFontSize();
    ttstr chStr = ttstr() + ch;
    int advance_width = measureTextWidth(chStr, effectiveFontSize);

    if(!m_options.ignoreDelay && m_currentChSpeed > 0) {
        if(m_options.widthTimeScale && effectiveFontSize > 0) {
            m_delayAccum += std::max(
                1, m_currentChSpeed * advance_width / effectiveFontSize);
        } else {
            m_delayAccum += m_currentChSpeed;
        }
        m_state.renderDelay = std::max(m_state.renderDelay, m_delayAccum);
    }

    CharacterInfo info{};
    info.bold = m_state.bold;
    info.italic = m_state.italic;
    info.graph = false;
    info.vertical = m_vertical;
    info.face = m_state.face;
    info.cw = advance_width;
    info.size = effectiveFontSize;
    info.pitch = getEffectivePitch();
    info.color = m_state.chColor;
    info.edge = m_state.edge;
    info.edgeColor = m_state.edgeColor;
    info.shadow = m_state.shadow;
    info.shadowColor = m_state.shadowColor;
    info.shadowDiff = m_state.shadowDiff;
    info.delay = m_delayAccum;
    info.link = m_currentLink;
    info.linkName = m_currentLinkName;
    info.text = chStr;

    bool hasRuby = !m_pendingRuby.IsEmpty() && m_pendingRubyCount > 0;
    ttstr rubyToAttach;
    int rubySpan = 0;
    if(hasRuby) {
        rubyToAttach = m_pendingRuby;
        rubySpan = m_pendingRubyCount;
        m_pendingRuby.Clear();
        m_pendingRubyCount = 0;
    }

    placeCharacterInfo(std::move(info));

    if(hasRuby && !m_currentLine.empty()) {
        attachPendingRuby(m_currentLine.size() - 1,
                          static_cast<size_t>(rubySpan), rubyToAttach);
    }
}

void TextRenderBase::setRenderSize(int width, int height) {
    m_boxWidth = width;
    m_boxHeight = height;

    dbg_print(
        TVPFormatMessage(TJS_W("set render size: (%1, %2)"), width, height));

    clear();
}

void TextRenderBase::setDefault(tTJSVariant defaultSettings) {
    dbg_print(TJS_W("set default format"));
    m_default.deserialize(defaultSettings);
    if(auto dict = defaultSettings.AsObjectNoAddRef()) {
        bool wb = m_default.wordBreak;
        if(getDictBool(dict, TJS_W("wordBreak"), wb) ||
           getDictBool(dict, TJS_W("wordbreak"), wb) ||
           getDictBool(dict, TJS_W("word_break"), wb)) {
            m_default.wordBreak = wb;
            m_state.wordBreak = wb;
            m_options.wordBreak = wb;
        }
    }
    resetFont();
    resetStyle();
}

void TextRenderBase::setFont(tTJSVariant fontSettings) {
    if(auto dict = fontSettings.AsObjectNoAddRef()) {
        m_state.deserializeFont(dict);
        updateFont();
    }
}

void TextRenderBase::setStyle(tTJSVariant styleSettings) {
    if(auto dict = styleSettings.AsObjectNoAddRef()) {
        m_state.deserializeStyle(dict);
    }
}

void TextRenderBase::setOption(tTJSVariant options) {
    dbg_print(TJS_W("set option"));
    m_options.deserialize(options);
    if(auto dict = options.AsObjectNoAddRef()) {
        getDictBool(dict, TJS_W("vertical"), m_vertical);
        bool wb = m_options.wordBreak;
        if(getDictBool(dict, TJS_W("word_break"), wb) ||
           getDictBool(dict, TJS_W("wordbreak"), wb) ||
           getDictBool(dict, TJS_W("wordBreak"), wb)) {
            m_options.wordBreak = wb;
            m_default.wordBreak = wb;
            m_state.wordBreak = wb;
        }
    }
}

tjs_error TextRenderBase::rawGetCharacters(tTJSVariant *result,
                                           tjs_int numparams,
                                           tTJSVariant **param,
                                           TextRenderBase *self) {
    int start = (numparams >= 1 && param[0]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[0])
        : 0;
    int count = (numparams >= 2 && param[1]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[1])
        : 0;
    tTJSVariant chars = self->getCharacters(start, count);
    if(result) {
        *result = chars;
    }
    return TJS_S_OK;
}

tTJSVariant TextRenderBase::getCharacters(int start, int count) {
    auto array = TJSCreateArrayObject();
    int total = static_cast<int>(m_characters.size());
    if(start < 0) {
        start = 0;
    }
    int end = (count <= 0) ? total : std::min(total, start + count);

    for(int i = start, outIdx = 0; i < end; ++i, ++outIdx) {
        auto ch = m_characters[static_cast<size_t>(i)].serialize();
        array->PropSetByNum(TJS_MEMBERENSURE, outIdx, &ch, array);
    }

    tTJSVariant res(array, array);
    array->Release();
    return res;
}

void TextRenderBase::clear() {
    dbg_print(TJS_W("clear character buffer and format"));

    m_characters.clear();
    m_currentLine.clear();
    m_keyWait.clear();

    double savedFontScale = m_state.fontScale;
    bool savedWordBreak = m_options.wordBreak;
    m_state = m_default;
    m_state.fontScale = savedFontScale;
    m_state.wordBreak = savedWordBreak;
    m_state.renderOver = false;
    m_state.renderDelay = 0;
    m_state.renderText.Clear();

    m_overflow = false;

    m_x = 0;
    m_y = 0;
    m_renderLeft = 0;
    m_renderTop = 0;
    m_renderRight = 0;
    m_renderBottom = 0;
    m_renderLines = 0;

    m_indent = 0;
    m_autoIndent = 0;
    m_autoIndentEndChar = 0;
    m_isBeginningOfLine = true;

    m_baseChSpeed = 0;
    m_currentChSpeed = 0;
    m_delayAccum = 0;
    m_currentLink.Clear();
    m_currentLinkName.Clear();
    m_pendingRuby.Clear();
    m_pendingRubyCount = 0;

    m_prevWasStick = false;
    m_wordStartIdx = -1;
    m_followingHangCount = 0;

    updateFont();
}

void TextRenderBase::updateFont() {
    if(m_self && !m_inUpdateFont) {
        m_inUpdateFont = true;
        ttstr prevFace = m_state.face;
        auto callOnFontChange = [&]() {
            auto dict = TJSCreateDictionaryObject();
            tTJSVariant vFace(m_state.face);
            tTJSVariant vBold(m_state.bold ? 1 : 0);
            tTJSVariant vItalic(m_state.italic ? 1 : 0);
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("face"), nullptr, &vFace,
                          dict);
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("bold"), nullptr, &vBold,
                          dict);
            dict->PropSet(TJS_MEMBERENSURE, TJS_W("italic"), nullptr, &vItalic,
                          dict);
            tTJSVariant arg(dict, dict);
            dict->Release();
            tTJSVariant *argP = &arg;
            m_self->FuncCall(0, TJS_W("onFontChange"), nullptr, nullptr, 1,
                             &argP, m_self);
        };
        callOnFontChange();
        if(m_state.face != prevFace) {
            callOnFontChange();
        }
        m_inUpdateFont = false;
    }

    auto rasterizer = GetCurrentRasterizer();
    if(!rasterizer) {
        return;
    }

    ttstr effectiveFace = m_state.face;
    if(effectiveFace == TJS_W("user") && m_self) {
        ttstr userFace;
        if(getDictString(m_self, TJS_W("userFace"), userFace) &&
           !userFace.IsEmpty()) {
            effectiveFace = userFace;
        }
    }

    auto font = tTVPFont{
        getEffectiveFontSize(),
        static_cast<tjs_uint32>((m_state.bold ? TVP_TF_BOLD : 0) |
                                (m_state.italic ? TVP_TF_ITALIC : 0)),
        0,
        effectiveFace,
    };

    rasterizer->ApplyFont(font);
}

void TextRenderBase::done() {
    dbg_print(TJS_W("flush character buffer"));
    flush(true);
}

void TextRenderBase::newline() {
    m_indent = 0;
    m_autoIndentEndChar = 0;
    commitCurrentLine(true);
}

void TextRenderBase::resetFont() {
    m_state.bold = m_default.bold;
    m_state.italic = m_default.italic;
    m_state.face = m_default.face;
    m_state.fontSize = m_default.fontSize;
    m_state.bigFontSize = m_default.bigFontSize;
    m_state.smallFontSize = m_default.smallFontSize;
    m_state.chColor = m_default.chColor;
    m_state.rubySize = m_default.rubySize;
    m_state.rubyOffset = m_default.rubyOffset;
    m_state.shadow = m_default.shadow;
    m_state.shadowColor = m_default.shadowColor;
    m_state.shadowDiff = m_default.shadowDiff;
    m_state.edge = m_default.edge;
    m_state.edgeColor = m_default.edgeColor;
    updateFont();
}

void TextRenderBase::resetStyle() {
    m_state.lineSpacing = m_default.lineSpacing;
    m_state.pitch = m_default.pitch;
    m_state.lineSize = m_default.lineSize;
    m_state.align = m_default.align;
    m_state.valign = m_default.valign;
    m_state.wordBreak = m_default.wordBreak;
}

tTJSVariant TextRenderBase::getKeyWait() {
    iTJSDispatch2 *dsp = TJSCreateArrayObject();
    for(size_t i = 0; i < m_keyWait.size(); ++i) {
        tTJSVariant kw = m_keyWait[i].serialize();
        dsp->PropSetByNum(TJS_MEMBERENSURE, static_cast<tjs_int>(i), &kw, dsp);
    }
    tTJSVariant ret(dsp, dsp);
    dsp->Release();
    return ret;
}

tjs_error TextRenderBase::rawCalcShowCount(tTJSVariant *result,
                                           tjs_int numparams,
                                           tTJSVariant **param,
                                           TextRenderBase *self) {
    int elapsed = (numparams >= 1 && param[0]->Type() != tvtVoid)
        ? static_cast<tjs_int>(*param[0])
        : 0;
    int cnt = self->calcShowCount(elapsed);
    if(result) {
        *result = cnt;
    }
    return TJS_S_OK;
}

int TextRenderBase::calcShowCount(int elapsed) {
    if(m_options.ignoreDelay || m_state.renderDelay <= 0) {
        return static_cast<int>(m_characters.size());
    }
    int count = 0;
    for(auto const &ch : m_characters) {
        if(ch.delay <= elapsed) {
            ++count;
        } else {
            break;
        }
    }
    return count;
}

tTJSVariant TextRenderBase::getLinkRects(tTJSVariant link) {
    ttstr target(link);
    iTJSDispatch2 *arr = TJSCreateArrayObject();
    int outIdx = 0;
    for(auto const &ch : m_characters) {
        if(!ch.link.IsEmpty() && (target.IsEmpty() || ch.link == target)) {
            auto dict = TJSCreateDictionaryObject();
            int left = ch.x;
            int top = ch.y;
            int width = ch.cw;
            int height = ch.size;
            int right = left + width;
            int bottom = top + height;
            setprop(dict, left);
            setprop(dict, top);
            setprop(dict, width);
            setprop(dict, height);
            setprop(dict, right);
            setprop(dict, bottom);
            tTJSVariant rv(dict, dict);
            dict->Release();
            arr->PropSetByNum(TJS_MEMBERENSURE, outIdx++, &rv, arr);
        }
    }
    tTJSVariant ret(arr, arr);
    arr->Release();
    return ret;
}

bool TextRenderBase::isLinkContains(tTJSVariant link, int x, int y) {
    ttstr target(link);
    for(auto const &ch : m_characters) {
        if(!ch.link.IsEmpty() && (target.IsEmpty() || ch.link == target)) {
            if(x >= ch.x && x < ch.x + ch.cw && y >= ch.y &&
               y < ch.y + ch.size) {
                return true;
            }
        }
    }
    return false;
}

tTJSVariant TextRenderBase::getLinkOfPosition(int x, int y) {
    for(auto const &ch : m_characters) {
        if(!ch.link.IsEmpty() && x >= ch.x && x < ch.x + ch.cw && y >= ch.y &&
           y < ch.y + ch.size) {
            auto dict = TJSCreateDictionaryObject();
            ttstr link = ch.link;
            ttstr name = ch.linkName;
            setprop(dict, link);
            setprop(dict, name);
            tTJSVariant ret(dict, dict);
            dict->Release();
            return ret;
        }
    }
    return tTJSVariant();
}

tTJSVariant TextRenderBase::getLinkNames() {
    iTJSDispatch2 *arr = TJSCreateArrayObject();
    int outIdx = 0;
    std::vector<ttstr> seen{};
    for(auto const &ch : m_characters) {
        if(!ch.link.IsEmpty() &&
           std::find(seen.begin(), seen.end(), ch.link) == seen.end()) {
            seen.push_back(ch.link);
            tTJSVariant v(ch.link);
            arr->PropSetByNum(TJS_MEMBERENSURE, outIdx++, &v, arr);
        }
    }
    tTJSVariant ret(arr, arr);
    arr->Release();
    return ret;
}

tTJSVariant TextRenderBase::getLinkCharacters(tTJSVariant link) {
    ttstr target(link);
    iTJSDispatch2 *arr = TJSCreateArrayObject();
    int outIdx = 0;
    for(auto const &ch : m_characters) {
        if(!ch.link.IsEmpty() && (target.IsEmpty() || ch.link == target)) {
            tTJSVariant cv = ch.serialize();
            arr->PropSetByNum(TJS_MEMBERENSURE, outIdx++, &cv, arr);
        }
    }
    tTJSVariant ret(arr, arr);
    arr->Release();
    return ret;
}

bool TextRenderBase::contains(int x, int y) const {
    return x >= m_renderLeft && x < m_renderRight && y >= m_renderTop &&
        y < m_renderBottom;
}

int TextRenderBase::calcLineOffset(int line) const {
    int step = std::max(getEffectiveLineSize(), getEffectiveFontSize()) +
        getEffectiveLineSpacing();
    return std::max(0, line) * step;
}

tTJSVariant TextRenderBase::findLink(int /*idx*/) { return tTJSVariant(); }

// register the class
NCB_REGISTER_CLASS(TextRenderBase) {
    Factory(&Class::factory);

    RawCallback(TJS_W("render"), &Class::rawRender, 0);
    NCB_METHOD(setRenderSize);
    NCB_METHOD(setDefault);
    NCB_METHOD(setFont);
    NCB_METHOD(setStyle);
    NCB_METHOD(setOption);
    RawCallback(TJS_W("getCharacters"), &Class::rawGetCharacters, 0);
    NCB_METHOD(clear);
    NCB_METHOD(done);
    NCB_METHOD(newline);

    NCB_METHOD(resetFont);
    NCB_METHOD(resetStyle);
    NCB_METHOD(getKeyWait);
    RawCallback(TJS_W("calcShowCount"), &Class::rawCalcShowCount, 0);
    NCB_METHOD(calcLineOffset);
    NCB_METHOD(contains);
    NCB_METHOD(getLinkNames);
    NCB_METHOD(getLinkCharacters);
    NCB_METHOD(getLinkRects);
    NCB_METHOD(isLinkContains);
    NCB_METHOD(getLinkOfPosition);
    NCB_METHOD(findLink);

    Property(TJS_W("keyWait"), &Class::getKeyWait, &Class::setKeyWait);
    property_delegate(renderCount);
    property_delegate(renderLines);
    property_delegate(renderOver);
    property_delegate(renderDelay);
    property_delegate(renderText);
    property_delegate(renderLeft);
    property_delegate(renderRight);
    property_delegate(renderTop);
    property_delegate(renderBottom);
    property_delegate(maxScrollOffset);
    property_delegate(maxScrollLine);

    property_delegate(vertical);
    property_delegate(bold);
    property_delegate(italic);
    property_delegate(face);
    property_delegate(fontSize);
    property_delegate(bigFontSize);
    property_delegate(smallFontSize);
    property_delegate(fontScale);
    property_delegate(timeScale);
    property_delegate(chColor);
    property_delegate(rubySize);
    property_delegate(rubyOffset);
    property_delegate(shadow);
    property_delegate(shadowColor);
    property_delegate(shadowDiff);
    property_delegate(edge);
    property_delegate(edgeColor);
    property_delegate(lineSpacing);
    property_delegate(pitch);
    property_delegate(lineSize);
    property_delegate(align);
    property_delegate(valign);
    property_delegate(wordBreak);

    property_delegate(defaultBold);
    property_delegate(defaultItalic);
    property_delegate(defaultFace);
    property_delegate(defaultFontSize);
    property_delegate(defaultBigFontSize);
    property_delegate(defaultSmallFontSize);
    property_delegate(defaultFontScale);
    property_delegate(defaultChColor);
    property_delegate(defaultRubySize);
    property_delegate(defaultRubyOffset);
    property_delegate(defaultShadow);
    property_delegate(defaultShadowColor);
    property_delegate(defaultShadowDiff);
    property_delegate(defaultEdge);
    property_delegate(defaultEdgeColor);
    property_delegate(defaultLineSpacing);
    property_delegate(defaultPitch);
    property_delegate(defaultLineSize);
    property_delegate(defaultAlign);
    property_delegate(defaultVAlign);
    property_delegate(defaultValign);
    property_delegate(defaultWordBreak);
};
