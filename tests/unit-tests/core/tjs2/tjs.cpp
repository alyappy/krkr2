//
// Created by LiDong on 2025/8/28.
//

#include <fstream>
#include <catch2/catch_test_macros.hpp>

#include "DebugImpl.h"
#include "FontImpl.h"
#include "Platform.h"
#include "test_config.h"
#include "TextStream.h"
#include "tjsBinarySerializer.h"
#include "tjsScriptBlock.h"
#include "TVPLog.h"
#include "UtilStreams.h"

class TJSConsoleOutputDef final : public iTJSConsoleOutput {
public:
    void ExceptionPrint(const tjs_char *msg) override {
        TVPLog(TVPLogLevel::Fatal, "tjs2", ttstr{ msg }.AsStdString());
    }

    void Print(const tjs_char *msg) override {
        TVPLog(TVPLogLevel::Info, "tjs2", ttstr{ msg }.AsStdString());
    }
} static iTJSConsoleOutputDef{};


TEST_CASE("exec tjs2 script") {
    const auto tvPScriptEngine = new tTJS();
    tvPScriptEngine->SetPPValue(TJS_W("krkr2"), 1);
    tvPScriptEngine->SetConsoleOutput(&iTJSConsoleOutputDef);

    // register some TVP classes/objects/functions/propeties
    tTJSVariant val;
    iTJSDispatch2 *dsp;
    iTJSDispatch2 *global = tvPScriptEngine->GetGlobalNoAddRef();

#define REGISTER_OBJECT(classname, instance)                                   \
    dsp = (instance);                                                          \
    val = tTJSVariant(dsp /*, dsp*/);                                          \
    dsp->Release();                                                            \
    global->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP, TJS_W(#classname),      \
                    nullptr, &val, global);

    REGISTER_OBJECT(Debug, TVPCreateNativeClass_Debug());

#define SCRIPT(file_name)                                                      \
    do {                                                                       \
        ttstr tvPInitTJSScriptText{};                                          \
        auto *stream = TVPCreateTextStreamForRead(                             \
            TEST_FILES_PATH "/tjs2/" file_name, "");                           \
        stream->Read(tvPInitTJSScriptText, 0);                                 \
        tvPScriptEngine->ExecScript(tvPInitTJSScriptText);                     \
        delete stream;                                                         \
    } while(false)

    SECTION("exec test_class.tjs") { SCRIPT("test_class.tjs"); }

    SECTION("exec test_function.tjs") { SCRIPT("test_function.tjs"); }

    SECTION("exec test_misc.tjs") { SCRIPT("test_misc.tjs"); }

    SECTION("exec test_string.tjs") { SCRIPT("test_string.tjs"); }

    SECTION("exec test_variant.tjs") { SCRIPT("test_variant.tjs"); }

    SECTION("exec test_with.tjs") { SCRIPT("test_with.tjs"); }

    // SECTION("exec test.tjs") {
    //     SCRIPT("test.tjs");
    // }

    tvPScriptEngine->Release();
}

TEST_CASE(
    "tTJSBinarySerializer decodes negative fixnums and empty/scalar dict keys",
    "[core][tjs2]") {
    const auto engine = new tTJS();
    engine->SetConsoleOutput(&iTJSConsoleOutputDef);

    // Dictionary with 3 entries:
    //   1) key "n" (fixstr len 1) -> negative fixnum -1 (0xFF)
    //   2) key ""  (fixstr len 0) -> positive fixnum 7 (ignored cleanly)
    //   3) key 42  (pos fixnum 42) -> negative fixnum -32 (0xE0)
    const tjs_uint8 payload[] = {
        0x83, // TYPE_FIX_DICTIONARY_MIN + 3
        0xA1, 'n', 0x00, // fixstr "n" (UTF-16LE)
        0xFF, // negative fixnum -1
        0xA0, // empty fixstr ""
        0x07, // positive fixnum 7
        0x2A, // positive fixnum 42 as scalar key
        0xE0, // negative fixnum -32
    };
    tTVPMemoryStream stream(payload, sizeof(payload));
    tTJSBinarySerializer serializer;
    tTJSVariant *root = serializer.Read(&stream);
    REQUIRE(root != nullptr);
    REQUIRE(root->Type() == tvtObject);
    iTJSDispatch2 *dict = root->AsObjectNoAddRef();
    REQUIRE(dict != nullptr);

    tTJSVariant vNeg1;
    REQUIRE(TJS_SUCCEEDED(
        dict->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("n"), nullptr, &vNeg1, dict)));
    REQUIRE(vNeg1.AsInteger() == -1);

    tTJSVariant vNeg32;
    REQUIRE(TJS_SUCCEEDED(dict->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("42"),
                                        nullptr, &vNeg32, dict)));
    REQUIRE(vNeg32.AsInteger() == -32);

    delete root;
    engine->Release();
}
