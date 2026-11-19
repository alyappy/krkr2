//
// Created by lidong on 25-6-21.
//

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "PluginIntf.h"
#include "ScriptMgnIntf.h"
#include "StorageIntf.h"
#include "SysInitIntf.h"
#include "UtilStreams.h"
#include "XP3Archive.h"
#include "ncbind.hpp"
#include "psbfile/PSBFile.h"
#include "psbfile/resources/ImageMetadata.h"
#include "test_config.h"
#include "tjs.h"

TEST_CASE("read psbfile ezsave.pimg") {
    PSB::PSBFile f;
    REQUIRE(f.loadPSBFile(TEST_FILES_PATH "/emote/ezsave.pimg"));
    const PSB::PSBHeader &header = f.getPSBHeader();
    REQUIRE(f.getType() == PSB::PSBType::Pimg);
    CAPTURE(header.version, f.getType());

    auto objs = f.getObjects();

    SECTION("check width height") {
        int width = static_cast<int>(
            *std::dynamic_pointer_cast<PSB::PSBNumber>((*objs)["width"]));
        int height = static_cast<int>(
            *std::dynamic_pointer_cast<PSB::PSBNumber>((*objs)["height"]));
        REQUIRE(width == 1280);
        REQUIRE(height == 720);
    }

    SECTION("get layers") {
        auto layers =
            std::dynamic_pointer_cast<PSB::PSBList>((*objs)["layers"]);
        REQUIRE(layers->size() == 32);

        SECTION("check layer properties") {
            std::vector group_layer_ids = { 3093, 3093, 3093, 2174, 2174, 2174,
                                            2174, 2158, 2158, 2158, 2158, 2158,
                                            2158, 2158, 2158, 2158, 2158, 2158,
                                            2158, 2158, 0,    2142, 2142, 2142,
                                            2135, 2135, 0,    0,    0,    0,
                                            0,    0 };

            std::vector heights = { 42,  42,  54,  43, 49,  49, 51, 51,
                                    51,  51,  51,  51, 51,  51, 51, 51,
                                    51,  51,  51,  51, 612, 42, 42, 54,
                                    612, 720, 720, 0,  0,   0,  0,  0 };

            std::vector widths = { 27, 27, 36, 34, 41,   41, 40, 36, 36, 36, 36,
                                   36, 36, 36, 36, 36,   36, 36, 36, 36, 40, 27,
                                   27, 36, 72, 80, 1280, 0,  0,  0,  0,  0 };

            std::vector names = { "@pageup:over",
                                  "@pageup:off",
                                  "@pageup:rect",
                                  "@item:thumb:rect",
                                  "@item:over",
                                  "@item:off",
                                  "@item:rect",
                                  "@item0/cp:item",
                                  "@item1/cp:item",
                                  "@item2/cp:item",
                                  "@item3/cp:item",
                                  "@item4/cp:item",
                                  "@item5/cp:item",
                                  "@item6/cp:item",
                                  "@item7/cp:item",
                                  "@item8/cp:item",
                                  "@item9/cp:item",
                                  "@item10/cp:item",
                                  "@item11/cp:item",
                                  "@item12/cp:item",
                                  "@scroll/lay:rect",
                                  "@pagedown:over",
                                  "@pagedown:off",
                                  "@pagedown:rect",
                                  "@base:open:rect",
                                  "@base:rect",
                                  "レイヤー 1",
                                  "pageup",
                                  "item",
                                  "items",
                                  "pagedown",
                                  "範囲情報" };

            std::vector layer_ids = { 3092, 3087, -1,   -1,   2168, 2164, -1,
                                      2157, 2156, 2155, 2154, 2153, 2152, 2151,
                                      2150, 2149, 2148, 2147, 2146, 2145, -1,
                                      2139, 2138, -1,   -1,   -1,   2036, 3093,
                                      2174, 2158, 2142, 2135 };

            std::vector layer_types = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 2, 2, 2, 2, 2 };

            std::vector lefts = {
                1249, 1249, 1244, 1246, 1239, 1239, 1240, 1244,
                1244, 1244, 1244, 1244, 1244, 1244, 1244, 1244,
                1244, 1244, 1244, 1244, 1240, 1248, 1248, 1244,
                1208, 1200, 0,    0,    0,    0,    0,    0,
            };

            std::vector tops = {
                7,   7,   0,   58,  55,  55,  54,  54,  105, 156, 207,
                258, 309, 360, 411, 462, 513, 564, 615, 666, 54,  671,
                671, 666, 54,  0,   0,   0,   0,   0,   0,   0,
            };

            std::vector visibles = {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
            };

            std::vector same_images = { 0,    0,    0,    0,    0,    0,
                                        0,    0,    2157, 2157, 2157, 2157,
                                        2157, 2157, 2157, 2157, 2157, 2157,
                                        2157, 2157, 0,    0,    0,    0,
                                        0,    0,    0,    0,    0,    0,
                                        0,    0 };

            for(int i = 0; i < layers->size(); i++) {
                auto layer =
                    std::dynamic_pointer_cast<PSB::PSBDictionary>((*layers)[i]);

                // height width opacity name layer_id layer_type left top type
                // visible  same_image
                {
                    auto group_layer_id =
                        std::dynamic_pointer_cast<PSB::PSBNumber>(
                            (*layer)["group_layer_id"]);
                    if(!(group_layer_id == nullptr &&
                         group_layer_ids[i] == 0)) {
                        REQUIRE(static_cast<int>(*group_layer_id) ==
                                group_layer_ids[i]);
                    }
                }
                {
                    auto height = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["height"]);
                    REQUIRE(static_cast<int>(*height) == heights[i]);
                }
                {
                    auto width = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["width"]);
                    REQUIRE(static_cast<int>(*width) == widths[i]);
                }
                {
                    auto opacity = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["opacity"]);
                    REQUIRE(static_cast<int>(*opacity) == 255);
                }
                {
                    auto name = std::dynamic_pointer_cast<PSB::PSBString>(
                        (*layer)["name"]);
                    REQUIRE(name->value == names[i]);
                }
                {
                    auto layer_id = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["layer_id"]);
                    REQUIRE(static_cast<int>(*layer_id) == layer_ids[i]);
                }
                {
                    auto layer_type = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["layer_type"]);
                    REQUIRE(static_cast<int>(*layer_type) == layer_types[i]);
                }
                {
                    auto left = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["left"]);
                    REQUIRE(static_cast<int>(*left) == lefts[i]);
                }
                {
                    auto top = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["top"]);
                    REQUIRE(static_cast<int>(*top) == tops[i]);
                }
                {
                    auto type = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["type"]);
                    REQUIRE(static_cast<int>(*type) == 13);
                }
                {
                    auto visible = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["visible"]);
                    REQUIRE(static_cast<int>(*visible) == visibles[i]);
                }
                {
                    auto same_image = std::dynamic_pointer_cast<PSB::PSBNumber>(
                        (*layer)["same_image"]);
                    if(!(same_image == nullptr && same_images[i] == 0)) {
                        REQUIRE(static_cast<int>(*same_image) ==
                                same_images[i]);
                    }
                }
            }
        }
    }

    SECTION("collect resources") {
        auto resMetadata = f.getTypeHandler()->collectResources(f, true);
        REQUIRE(!resMetadata.empty());
        for(auto &res : resMetadata) {
            auto imgMetadata = dynamic_cast<PSB::ImageMetadata *>(res.get());
            REQUIRE(imgMetadata != nullptr);
        }
    }
}

TEST_CASE("read psbfile e-mote3.0 psb") {
    constexpr int key = 742877301;
    PSB::PSBFile f;
    f.setSeed(key);
    REQUIRE(
        f.loadPSBFile(TEST_FILES_PATH "/emote/e-mote3.0バニラパジャマa.psb"));
    REQUIRE(f.getType() == PSB::PSBType::Motion);
    auto objs = f.getObjects();
    REQUIRE(objs != nullptr);
}

TEST_CASE("texture index parsing preserves digit and overflow behavior",
          "[texture-index]") {
    const auto parse = [](const std::string &name) {
        PSB::ImageMetadata metadata;
        metadata.setPart(name);
        return metadata.getTextureIndex();
    };
    REQUIRE(parse("prefixtex") == 0);
    REQUIRE(parse("tex#000") == 0);
    REQUIRE(parse("tex000") == 0);
    REQUIRE(parse("tex#001") == 1);
    REQUIRE(parse("tex-no-digits-42-tail") == 42);
    REQUIRE(parse("tex12tex7") == 7);
    REQUIRE(parse("tex4294967295") == UINT32_MAX);
    REQUIRE_FALSE(parse("tex4294967296"));
    REQUIRE_FALSE(parse("tex999999999999999999999"));
    REQUIRE_FALSE(parse("tex-without-digits"));
    REQUIRE_FALSE(parse("other123"));
}

namespace {
    std::vector<std::uint8_t> buildMinimalPsb(std::uint16_t version) {
        std::vector<std::uint8_t> buf;
        auto pushU16 = [&](std::uint16_t v) {
            buf.push_back(static_cast<std::uint8_t>(v & 0xFF));
            buf.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        };
        auto pushU32 = [&](std::uint32_t v) {
            for(int i = 0; i < 4; ++i)
                buf.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
        };
        const std::uint32_t headerSize =
            PSB::PSBHeader::GetHeaderLength(version);
        // Empty PSBArray: ArrayN1 (0x0D), count=0, NumberN8 (0x0C => 0-width)
        const std::uint32_t emptyArrayOff = headerSize;
        const std::uint32_t namesOff = emptyArrayOff + 3;
        // charset=[0], namesData=[0], nameIndexes=[]
        const std::uint32_t entriesOff = namesOff + 12;
        const std::uint32_t stringsDataOff = entriesOff + 7;

        buf.insert(buf.end(), { 'P', 'S', 'B', '\0' });
        pushU16(version);
        pushU16(0); // encrypt
        pushU32(0); // offsetEncrypt
        pushU32(namesOff);
        pushU32(emptyArrayOff); // offsetStrings
        pushU32(stringsDataOff); // offsetStringsData
        pushU32(emptyArrayOff); // offsetChunkOffsets
        pushU32(emptyArrayOff); // offsetChunkLengths
        pushU32(stringsDataOff); // offsetChunkData
        pushU32(entriesOff); // offsetEntries
        if(version > 2) {
            pushU32(0); // checksum
        }
        if(version > 3) {
            pushU32(emptyArrayOff); // offsetExtraChunkOffsets
            pushU32(emptyArrayOff); // offsetExtraChunkLengths
            pushU32(stringsDataOff); // offsetExtraChunkData
        }
        REQUIRE(buf.size() == headerSize);

        // emptyArrayOff: empty 0-width PSBArray
        buf.insert(buf.end(), { 0x0D, 0x00, 0x0C });

        // namesOff: charset=[0] (4B), namesData=[0] (4B), nameIndexes=[] (3B) +
        // 1B pad
        buf.insert(buf.end(), { 0x0D, 0x01, 0x0D, 0x00 });
        buf.insert(buf.end(), { 0x0D, 0x01, 0x0D, 0x00 });
        buf.insert(buf.end(), { 0x0D, 0x00, 0x0C, 0x00 });

        // entriesOff: empty Objects dictionary (0x21 + empty names + empty
        // offsets)
        buf.insert(buf.end(), { 0x21, 0x0D, 0x00, 0x0C, 0x0D, 0x00, 0x0C });

        // stringsDataOff: terminator byte
        buf.push_back(0x00);
        return buf;
    }
} // namespace

TEST_CASE(
    "PSBArray accepts empty array with zero-width entry type in scn files",
    "[plugins][psbfile]") {
    const std::uint8_t rawEmpty[] = { 0x00, 0x0C }; // count=0, entryLength=0
    tTVPMemoryStream emptyStream(rawEmpty, sizeof(rawEmpty));
    PSB::PSBArray arr(1, &emptyStream);
    REQUIRE(arr.value.empty());
    REQUIRE(arr.entryLength == 0);

    const std::uint8_t rawNonEmptyZeroWidth[] = { 0x01, 0x0C };
    tTVPMemoryStream badStream(rawNonEmptyZeroWidth,
                               sizeof(rawNonEmptyZeroWidth));
    REQUIRE_THROWS_AS(PSB::PSBArray(1, &badStream), std::runtime_error);
}

TEST_CASE("PSBFile loads version 4 stream and rejects version 5",
          "[plugins][psbfile]") {
    auto v4Bytes = buildMinimalPsb(4);
    tTVPMemoryStream v4Stream(v4Bytes.data(), v4Bytes.size());
    PSB::PSBFile v4File;
    REQUIRE(v4File.loadPSBStream(&v4Stream));
    REQUIRE(v4File.getPSBHeader().version == 4);
    REQUIRE(v4File.getObjects() != nullptr);

    auto v5Bytes = buildMinimalPsb(5);
    tTVPMemoryStream v5Stream(v5Bytes.data(), v5Bytes.size());
    PSB::PSBFile v5File;
    REQUIRE_FALSE(v5File.loadPSBStream(&v5Stream));
}

extern tTVPXP3ArchiveExtractionFilter TVPXP3ArchiveExtractionFilter;

TEST_CASE("xp3filter registers Storages.setXP3ArchiveExtractionFilter on main "
          "script engine",
          "[plugins][xp3filter]") {
    const ttstr testDir = TVPNormalizeStorageName(ttstr(TEST_FILES_PATH "/"));
    TVPProjectDir = testDir;
    TVPSetCurrentDirectory(testDir);
    if(!TVPGetScriptEngine()) {
        TVPInitScriptEngine();
    }
    ncbAutoRegister::AllRegist();
    REQUIRE(ncbAutoRegister::LoadModule(TJS_W("xp3filter.dll")));

    TVPExecuteScript(
        TJS_W("Storages.setXP3ArchiveExtractionFilter(function(h, o, b, l) {"
              "  for(var i = 0; i < l; ++i) b[i] ^= 0x5A;"
              "});"));

    REQUIRE(TVPXP3ArchiveExtractionFilter != nullptr);
    unsigned char buf[4] = { 0x10, 0x20, 0x30, 0x40 };
    const ttstr name(TJS_W("test.bin"));
    tTVPXP3ExtractionFilterInfo info(0, buf, sizeof(buf), 0x1234, name);
    tTJSVariant ctx;
    TVPXP3ArchiveExtractionFilter(&info, &ctx);
    REQUIRE(buf[0] == (0x10 ^ 0x5A));
    REQUIRE(buf[1] == (0x20 ^ 0x5A));
    REQUIRE(buf[2] == (0x30 ^ 0x5A));
    REQUIRE(buf[3] == (0x40 ^ 0x5A));

    TVPExecuteScript(TJS_W("Storages.setXP3ArchiveExtractionFilter(void);"));
    REQUIRE(TVPXP3ArchiveExtractionFilter == nullptr);
}
