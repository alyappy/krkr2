#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "PluginIntf.h"
#include "ScriptMgnIntf.h"
#include "ncbind.hpp"

extern tTJS *TVPScriptEngine;

namespace {

    std::string evalTjsString(const ttstr &expr) {
        tTJSVariant res;
        TVPExecuteExpression(expr, &res);
        return ttstr(res).AsStdString();
    }

    int evalTjsInt(const ttstr &expr) {
        tTJSVariant res;
        TVPExecuteExpression(expr, &res);
        return static_cast<tjs_int>(res);
    }

} // namespace

TEST_CASE("TextRenderBase auto wordwrap and kinsoku") {
    if(!TVPScriptEngine) {
        TVPScriptEngine = new tTJS();
    }
    ncbAutoRegister::AllRegist();
    ncbAutoRegister::LoadModule(TJS_W("TextRender.dll"));

    TVPExecuteScript(TJS_W(R"(
        class MockTextRender extends TextRenderBase {
            var fixedCharWidth = 12;
            function MockTextRender() {
                super.TextRenderBase();
            }
            function onFontChange(info) {
                if (info.face == "user" && defaultFace != "user") {
                    global.TextRenderBase.setFont(%[face: defaultFace]);
                }
            }
            function onGetTextWidth(text, size) {
                return (fixedCharWidth > 0) ? (fixedCharWidth * text.length) : (size / 2 * text.length);
            }
            function getLines() {
                var chars = getCharacters(0, 0);
                var lines = [];
                var currentY = void;
                var currentLine = "";
                for (var i = 0; i < chars.count; ++i) {
                    var ch = chars[i];
                    if (currentY === void || ch.y != currentY) {
                        if (currentY !== void) {
                            lines.add(currentLine);
                        }
                        currentY = ch.y;
                        currentLine = ch.text;
                    } else {
                        currentLine += ch.text;
                    }
                }
                if (currentY !== void) {
                    lines.add(currentLine);
                }
                return lines;
            }
        }
    )"));

    SECTION("word_break=0 wraps entire words without leaving first character "
            "on previous line") {
        // Exact sentence from user screenshot where 61 chars ("Though ...
        // failure, i") fit in 61 * 12 = 732px when boxWidth = 735px. With
        // word_break=0, "it's", "sponsorship", and "willing" must wrap as whole
        // words.
        TVPExecuteScript(TJS_W(R"(
            var tr = new MockTextRender();
            tr.fixedCharWidth = 12;
            tr.setRenderSize(735, 300);
            tr.setOption(%[word_break: 0, width_time_scale: 1]);
            var text = "Though it went down in anime history as a complete failure, it's unclear whether the bigwigs behind the regional sponsorship deal are clueless to that fact or simply aren't willing to admit it.";
            tr.render(text, 0, 0, 0, 0, 0);
            tr.done();
            global.testLines = tr.getLines();
            global.testChars = tr.getCharacters(0, 0);
        )"));

        REQUIRE(evalTjsInt(TJS_W("global.testLines.count")) == 4);
        REQUIRE(evalTjsString(TJS_W("global.testLines[0]")) ==
                "Though it went down in anime history as a complete failure, ");
        REQUIRE(evalTjsString(TJS_W("global.testLines[1]")) ==
                "it's unclear whether the bigwigs behind the regional ");
        REQUIRE(evalTjsString(TJS_W("global.testLines[2]")) ==
                "sponsorship deal are clueless to that fact or simply aren't ");
        REQUIRE(evalTjsString(TJS_W("global.testLines[3]")) ==
                "willing to admit it.");

        // Check that wrapped lines start at x == 0
        REQUIRE(evalTjsInt(TJS_W("global.testChars[60].x")) == 0);
        REQUIRE(evalTjsString(TJS_W("global.testChars[60].text")) == "i");

        // Verify renderText contains newline-separated lines (used by
        // rendermsgwin.tjs:561)
        REQUIRE(evalTjsInt(TJS_W(
                    "global.trRenderLinesCount = global.testLines.count")) ==
                4);
    }

    SECTION("word_break=0 handles overflow at space, first char of word, and "
            "oversized word") {
        TVPExecuteScript(TJS_W(R"(
            var tr = new MockTextRender();
            tr.fixedCharWidth = 10;
            // 100px fits 10 chars per line
            tr.setRenderSize(100, 300);
            tr.setOption(%[word_break: 0]);
            tr.render("hello world supercalifragilistic ok", 0, 0, 0, 0, 0);
            tr.done();
            global.testLines2 = tr.getLines();
        )"));

        REQUIRE(evalTjsInt(TJS_W("global.testLines2.count")) == 5);
        REQUIRE(evalTjsString(TJS_W("global.testLines2[0]")) == "hello ");
        REQUIRE(evalTjsString(TJS_W("global.testLines2[1]")) == "world ");
        REQUIRE(evalTjsString(TJS_W("global.testLines2[2]")) == "supercalif");
        REQUIRE(evalTjsString(TJS_W("global.testLines2[3]")) == "ragilistic ");
        REQUIRE(evalTjsString(TJS_W("global.testLines2[4]")) == "ok");
    }

    SECTION(
        "getCharacters(start, count), newline(), and fontScale preservation") {
        TVPExecuteScript(TJS_W(R"(
            var tr = new MockTextRender();
            tr.fixedCharWidth = 0; // use size / 2
            tr.defaultFontSize = 24;
            tr.setRenderSize(200, 200);
            tr.fontScale = 0.5;
            tr.resetStyle();
            tr.clear();
            tr.resetFont();
            tr.setOption(%[word_break: 0]);
            tr.render("Hello world", 0, 10, 0, 0, 0);
            tr.newline();
            tr.render("Sub line", 0, 10, 0, 1, 0);
            tr.done();
            global.sliceChars = tr.getCharacters(6, 5);
            global.allChars = tr.getCharacters(0, 0);
            global.lineCount = tr.renderLines;
        )"));

        REQUIRE(evalTjsInt(TJS_W("global.lineCount")) == 2);
        REQUIRE(evalTjsInt(TJS_W("global.sliceChars.count")) == 5);
        REQUIRE(evalTjsString(TJS_W("global.sliceChars[0].text")) == "w");
        REQUIRE(evalTjsString(TJS_W("global.sliceChars[4].text")) == "d");
        // fontScale = 0.5 of defaultFontSize 24 => size 12
        REQUIRE(evalTjsInt(TJS_W("global.allChars[0].size")) == 12);
    }

    SECTION("TagTextConverter tags (%n;, %id, %bd, %sd, %ed, %;, %a, %t, %k, "
            "\\x, \\w, and trailing %i at EOF) parse cleanly without "
            "throwing") {
        TVPExecuteScript(TJS_W(R"(
            var tr = new MockTextRender();
            tr.fixedCharWidth = 10;
            tr.defaultFontSize = 24;
            tr.defaultItalic = false;
            tr.defaultBold = false;
            tr.setRenderSize(400, 300);
            tr.setOption(%[word_break: 0]);
            tr.render("Itu......%n;%i1Italic%idNormal%b1Bold%bd%s#112233;%e#445566;%150;Big%;%a20;%t50;\\x\\wEnd%i", 0, 10, 0, 0, 0);
            tr.done();
            global.tagTestLineCount = tr.renderLines;
            global.tagTestRenderText = tr.renderText;
            global.tagTestChars = tr.getCharacters(0, 0);
        )"));

        REQUIRE(evalTjsInt(TJS_W("global.tagTestLineCount")) == 2);
        REQUIRE(evalTjsString(TJS_W("global.tagTestRenderText")) ==
                "Itu......\nItalicNormalBoldBigEnd");
        // 'I' of "Italic" has italic == true, 'N' of "Normal" has italic ==
        // false (reset by %id)
        REQUIRE(evalTjsInt(TJS_W("global.tagTestChars[9].italic ? 1 : 0")) ==
                1);
        REQUIRE(evalTjsInt(TJS_W("global.tagTestChars[15].italic ? 1 : 0")) ==
                0);
        // 'B' of "Big" has size 36 (%150;), 'E' of "End" has size 24 (%;)
        REQUIRE(evalTjsInt(TJS_W("global.tagTestChars[25].size")) == 36);
        REQUIRE(evalTjsInt(TJS_W("global.tagTestChars[28].size")) == 24);
    }
}
