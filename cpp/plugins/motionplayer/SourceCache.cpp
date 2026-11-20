#include "SourceCache.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <unordered_set>
#include <vector>

#include "BitmapIntf.h"
#include "GraphicsLoaderIntf.h"
#include "LayerBitmapIntf.h"
#include "LayerIntf.h"
#include "PlayerInternal.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "ScriptMgnIntf.h"
#include "StorageIntf.h"

namespace {

    bool getObjectProperty(const tTJSVariant &object, const tjs_char *name,
                           tTJSVariant &out) {
        if(object.Type() != tvtObject || !object.AsObjectNoAddRef()) {
            return false;
        }
        return TJS_SUCCEEDED(object.AsObjectNoAddRef()->PropGet(
            0, name, nullptr, &out, object.AsObjectNoAddRef()));
    }

    std::optional<ttstr> sourceNameFromVariant(const tTJSVariant &value) {
        if(value.Type() == tvtVoid) {
            return std::nullopt;
        }
        if(value.Type() == tvtObject && value.AsObjectNoAddRef()) {
            for(const auto *name : { TJS_W("src"), TJS_W("key") }) {
                tTJSVariant prop;
                if(getObjectProperty(value, name, prop) &&
                   prop.Type() != tvtVoid) {
                    return ttstr(prop);
                }
            }
            return std::nullopt;
        }
        return ttstr(value);
    }

    bool packedColorsAreDefault(std::uint32_t c0, std::uint32_t c1,
                                std::uint32_t c2, std::uint32_t c3) {
        return c0 == 0xFF808080u && c1 == 0xFF808080u && c2 == 0xFF808080u &&
            c3 == 0xFF808080u;
    }

    bool packedColorsAreOpaqueWhite(std::uint32_t c0, std::uint32_t c1,
                                    std::uint32_t c2, std::uint32_t c3) {
        return (c0 & c1 & c2 & c3) == 0xFFFFFFFFu;
    }

    std::array<int, 4> unpackPackedRgba(std::uint32_t packedColor) {
        return {
            static_cast<int>(packedColor & 0xFFu),
            static_cast<int>((packedColor >> 8) & 0xFFu),
            static_cast<int>((packedColor >> 16) & 0xFFu),
            static_cast<int>((packedColor >> 24) & 0xFFu),
        };
    }

    constexpr std::size_t kMaxSourceCacheEntries = 512;
    constexpr std::size_t kMaxSourceCacheEntryBytes = 128u * 1024u * 1024u;
    constexpr std::size_t kMaxBaseBitmapEntries = 512;
    constexpr std::size_t kMaxBaseBitmapBytes = 96u * 1024u * 1024u;

    std::size_t bitmapByteSize(const tTVPBaseBitmap *bmp) {
        if(!bmp || bmp->GetWidth() <= 0 || bmp->GetHeight() <= 0) {
            return 0;
        }
        const std::size_t bpp = bmp->Is8BPP() ? 1u : 4u;
        return static_cast<std::size_t>(bmp->GetWidth()) *
            static_cast<std::size_t>(bmp->GetHeight()) * bpp;
    }

    bool packedColorsNeedTint(const std::array<std::uint32_t, 4> &c) {
        return !packedColorsAreDefault(c[0], c[1], c[2], c[3]) &&
            !packedColorsAreOpaqueWhite(c[0], c[1], c[2], c[3]);
    }

    inline std::uint8_t scaleChannelFast(int value, int tint,
                                         bool halfAlphaBlend) {
        const int prod = tint * value;
        const int scaled =
            halfAlphaBlend ? (prod >> 7) : ((prod + 1 + (prod >> 8)) >> 8);
        return static_cast<std::uint8_t>(scaled > 255 ? 255 : scaled);
    }

    void applyPackedCornerTintFromBaseLike_0x6A7518(
        const tTVPBaseBitmap &src, tTVPBaseBitmap &dst,
        const std::array<std::uint32_t, 4> &packedColors, bool halfAlphaBlend) {
        const int width = static_cast<int>(src.GetWidth());
        const int height = static_cast<int>(src.GetHeight());
        if(width <= 0 || height <= 0) {
            return;
        }

        const auto *srcBase =
            static_cast<const std::uint8_t *>(src.GetScanLine(0));
        auto *dstBase = static_cast<std::uint8_t *>(dst.GetScanLineForWrite(0));
        const std::ptrdiff_t srcPitch = src.GetPitchBytes();
        const std::ptrdiff_t dstPitch = dst.GetPitchBytes();
        if(!srcBase || !dstBase) {
            return;
        }

        const auto c0 = packedColors[0];
        const auto c1 = packedColors[1];
        const auto c2 = packedColors[2];
        const auto c3 = packedColors[3];
        if(packedColorsAreDefault(c0, c1, c2, c3) ||
           packedColorsAreOpaqueWhite(c0, c1, c2, c3)) {
            const size_t rowBytes = static_cast<size_t>(width) * 4u;
            for(int y = 0; y < height; ++y) {
                std::memcpy(dstBase + static_cast<std::ptrdiff_t>(y) * dstPitch,
                            srcBase + static_cast<std::ptrdiff_t>(y) * srcPitch,
                            rowBytes);
            }
            return;
        }

        const int colorDivisor = halfAlphaBlend ? 128 : 255;

        // Fast path: uniform tint across all 4 corners (typical fade/tint).
        if(c0 == c1 && c1 == c2 && c2 == c3) {
            const auto rgba = unpackPackedRgba(c0);
            std::uint8_t lutB[256];
            std::uint8_t lutG[256];
            std::uint8_t lutR[256];
            std::uint8_t lutA[256];
            for(int i = 0; i < 256; ++i) {
                lutR[i] = static_cast<std::uint8_t>(
                    std::min(255, rgba[0] * i / colorDivisor));
                lutG[i] = static_cast<std::uint8_t>(
                    std::min(255, rgba[1] * i / colorDivisor));
                lutB[i] = static_cast<std::uint8_t>(
                    std::min(255, rgba[2] * i / colorDivisor));
                lutA[i] = static_cast<std::uint8_t>(
                    std::min(255, rgba[3] * i / colorDivisor));
            }
            for(int y = 0; y < height; ++y) {
                const auto *srcRow =
                    srcBase + static_cast<std::ptrdiff_t>(y) * srcPitch;
                auto *dstRow =
                    dstBase + static_cast<std::ptrdiff_t>(y) * dstPitch;
                for(int x = 0; x < width; ++x) {
                    const auto *sp = srcRow + static_cast<size_t>(x) * 4u;
                    auto *dp = dstRow + static_cast<size_t>(x) * 4u;
                    dp[0] = lutB[sp[0]];
                    dp[1] = lutG[sp[1]];
                    dp[2] = lutR[sp[2]];
                    dp[3] = lutA[sp[3]];
                }
            }
            return;
        }

        const auto topLeft = unpackPackedRgba(c0);
        const auto topRight = unpackPackedRgba(c1);
        const auto bottomRight = unpackPackedRgba(c2);
        const auto bottomLeft = unpackPackedRgba(c3);
        const int spanX = std::max(width - 1, 1);
        const int spanY = std::max(height - 1, 1);
        const auto lerpChannel = [](int a, int b, int pos, int span) -> int {
            return a + (pos * (b - a)) / span;
        };

        for(int y = 0; y < height; ++y) {
            const auto *srcRow =
                srcBase + static_cast<std::ptrdiff_t>(y) * srcPitch;
            auto *dstRow = dstBase + static_cast<std::ptrdiff_t>(y) * dstPitch;
            const int rowLeftR =
                lerpChannel(topLeft[0], bottomLeft[0], y, spanY);
            const int rowLeftG =
                lerpChannel(topLeft[1], bottomLeft[1], y, spanY);
            const int rowLeftB =
                lerpChannel(topLeft[2], bottomLeft[2], y, spanY);
            const int rowLeftA =
                lerpChannel(topLeft[3], bottomLeft[3], y, spanY);
            const int rowRightR =
                lerpChannel(topRight[0], bottomRight[0], y, spanY);
            const int rowRightG =
                lerpChannel(topRight[1], bottomRight[1], y, spanY);
            const int rowRightB =
                lerpChannel(topRight[2], bottomRight[2], y, spanY);
            const int rowRightA =
                lerpChannel(topRight[3], bottomRight[3], y, spanY);

            const std::int64_t stepR =
                (static_cast<std::int64_t>(rowRightR - rowLeftR) << 16) / spanX;
            const std::int64_t stepG =
                (static_cast<std::int64_t>(rowRightG - rowLeftG) << 16) / spanX;
            const std::int64_t stepB =
                (static_cast<std::int64_t>(rowRightB - rowLeftB) << 16) / spanX;
            const std::int64_t stepA =
                (static_cast<std::int64_t>(rowRightA - rowLeftA) << 16) / spanX;
            std::int64_t curR = static_cast<std::int64_t>(rowLeftR) << 16;
            std::int64_t curG = static_cast<std::int64_t>(rowLeftG) << 16;
            std::int64_t curB = static_cast<std::int64_t>(rowLeftB) << 16;
            std::int64_t curA = static_cast<std::int64_t>(rowLeftA) << 16;

            for(int x = 0; x < width; ++x) {
                const auto *sp = srcRow + static_cast<size_t>(x) * 4u;
                auto *dp = dstRow + static_cast<size_t>(x) * 4u;
                dp[2] = scaleChannelFast(static_cast<int>(sp[2]),
                                         static_cast<int>(curR >> 16),
                                         halfAlphaBlend);
                dp[1] = scaleChannelFast(static_cast<int>(sp[1]),
                                         static_cast<int>(curG >> 16),
                                         halfAlphaBlend);
                dp[0] = scaleChannelFast(static_cast<int>(sp[0]),
                                         static_cast<int>(curB >> 16),
                                         halfAlphaBlend);
                dp[3] = scaleChannelFast(static_cast<int>(sp[3]),
                                         static_cast<int>(curA >> 16),
                                         halfAlphaBlend);
                curR += stepR;
                curG += stepG;
                curB += stepB;
                curA += stepA;
            }
        }
    }

    void pushGraphicCandidates(std::vector<ttstr> &candidates,
                               const ttstr &base) {
        if(base.IsEmpty()) {
            return;
        }

        candidates.push_back(base);
        const auto raw = motion::detail::narrow(base);
        if(raw.find('.') != std::string::npos) {
            return;
        }

        static const char *exts[] = { ".png", ".webp", ".jpg",  ".jpeg",
                                      ".bmp", ".tlg",  ".pimg", ".psb" };
        for(const auto *ext : exts) {
            candidates.emplace_back(base + ttstr{ ext });
        }
    }

    ttstr resolveMotionSourcePathLike_0x6948E8(
        const motion::detail::MotionSnapshot &snapshot,
        const std::string &source) {
        if(source.empty() || motion::internal::isMotionCrossReference(source)) {
            return {};
        }

        std::vector<ttstr> candidates;
        const auto sourcePath = motion::detail::widen(source);
        const auto lastSlash = source.rfind('/');
        const auto baseName = lastSlash == std::string::npos
            ? source
            : source.substr(lastSlash + 1);

        const auto appendCandidatesForSnapshot =
            [&](const motion::detail::MotionSnapshot &snap) {
                pushGraphicCandidates(candidates, sourcePath);
                motion::detail::appendEmbeddedSourceCandidates(snap, source,
                                                               candidates);
                for(const auto &alias : snap.resourceAliases) {
                    const auto embeddedBase = ttstr{ TJS_W("psb://") } +
                        motion::detail::widen(alias) + TJS_W("/") + sourcePath;
                    pushGraphicCandidates(candidates, embeddedBase);
                }
                for(const auto &[resPath, ignored] : snap.resourcesByPath) {
                    (void)ignored;
                    const auto targetSuffix = "/" + baseName + "/pixel";
                    if(resPath.size() >= targetSuffix.size() &&
                       resPath.compare(resPath.size() - targetSuffix.size(),
                                       targetSuffix.size(),
                                       targetSuffix) == 0) {
                        for(const auto &alias : snap.resourceAliases) {
                            const auto psbPath = ttstr{ TJS_W("psb://") } +
                                motion::detail::widen(alias) + TJS_W("/") +
                                motion::detail::widen(resPath);
                            pushGraphicCandidates(candidates, psbPath);
                        }
                    }
                }
            };

        appendCandidatesForSnapshot(snapshot);
        for(const auto &attached : snapshot.attachedSnapshots) {
            if(attached) {
                appendCandidatesForSnapshot(*attached);
            }
        }

        std::unordered_set<std::string> seen;
        for(const auto &candidate : candidates) {
            const auto candidateKey = motion::detail::narrow(candidate);
            if(!seen.insert(candidateKey).second || candidate.IsEmpty()) {
                continue;
            }
            if(candidateKey.rfind("psb://", 0) == 0) {
                if(TVPIsExistentStorage(candidate)) {
                    return candidate;
                }
                continue;
            }
            if(const auto placed = TVPGetPlacedPath(candidate);
               !placed.IsEmpty()) {
                return placed;
            }
        }
        return {};
    }

    std::shared_ptr<tTVPBaseBitmap> loadGraphicBitmap(const ttstr &path) {
        if(path.IsEmpty()) {
            return nullptr;
        }

        ttstr loadPath = path;
        const auto pathString = motion::detail::narrow(path);
        if(pathString.rfind('.') == std::string::npos ||
           pathString.rfind('.') < pathString.rfind('/')) {
            loadPath = path + TJS_W(".png");
        }

        try {
            auto bmp = std::make_shared<tTVPBaseBitmap>(1, 1, 32);
            TVPLoadGraphic(bmp.get(), loadPath, TVP_clNone, 0, 0, glmNormal,
                           nullptr, nullptr);
            if(bmp->GetWidth() > 0 && bmp->GetHeight() > 0) {
                return bmp;
            }
        } catch(...) {
        }
        return nullptr;
    }

    std::shared_ptr<tTVPBaseBitmap>
    loadPsbBitmap(const motion::detail::MotionSnapshot &snapshot,
                  const std::string &sourceKey) {
        int width = 0;
        int height = 0;
        double originX = 0.0;
        double originY = 0.0;
        std::vector<std::uint8_t> decodedPixels;
        bool decodedPixelsAreBgra = false;
        const auto *resource = motion::internal::findPSBResourceBySourceName(
            snapshot, sourceKey, width, height, decodedPixels, originX, originY,
            &decodedPixelsAreBgra);
        if(!resource || width <= 0 || height <= 0 || resource->data.empty()) {
            return nullptr;
        }

        const auto &pixelData =
            decodedPixels.empty() ? resource->data : decodedPixels;
        auto bmp = std::make_shared<tTVPBaseBitmap>(
            static_cast<tjs_uint>(width), static_cast<tjs_uint>(height), 32);
        const size_t fullBytes =
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
        if(pixelData.size() < fullBytes) {
            tTVPRect fillRect(0, 0, width, height);
            bmp->Fill(fillRect, 0x00000000);
        }
        auto *dstBase =
            static_cast<std::uint8_t *>(bmp->GetScanLineForWrite(0));
        const std::ptrdiff_t dstPitch = bmp->GetPitchBytes();
        const auto *src = pixelData.data();
        const size_t rowBytes = static_cast<size_t>(width) * 4u;
        if(decodedPixelsAreBgra && pixelData.size() >= fullBytes) {
            for(int y = 0; y < height; ++y) {
                std::memcpy(dstBase + static_cast<std::ptrdiff_t>(y) * dstPitch,
                            src + static_cast<size_t>(y) * rowBytes, rowBytes);
            }
        } else {
            for(int y = 0; y < height; ++y) {
                auto *row = dstBase + static_cast<std::ptrdiff_t>(y) * dstPitch;
                const size_t rowOffset = static_cast<size_t>(y) * rowBytes;
                if(rowOffset >= pixelData.size()) {
                    break;
                }
                const int availPixels = static_cast<int>(
                    std::min(static_cast<size_t>(width),
                             (pixelData.size() - rowOffset) / 4u));
                const auto *srcRow = src + rowOffset;
                if(decodedPixelsAreBgra) {
                    std::memcpy(row, srcRow,
                                static_cast<size_t>(availPixels) * 4u);
                } else {
                    for(int x = 0; x < availPixels; ++x) {
                        const auto *sp = srcRow + static_cast<size_t>(x) * 4u;
                        auto *dp = row + static_cast<size_t>(x) * 4u;
                        dp[0] = sp[2];
                        dp[1] = sp[1];
                        dp[2] = sp[0];
                        dp[3] = sp[3];
                    }
                }
            }
        }
        return bmp;
    }

    tTJSNI_BaseLayer *resolveNativeLayer(iTJSDispatch2 *layerObject) {
        if(!layerObject) {
            return nullptr;
        }
        tTJSNI_BaseLayer *layer = nullptr;
        if(TJS_FAILED(layerObject->NativeInstanceSupport(
               TJS_NIS_GETINSTANCE, tTJSNC_Layer::ClassID,
               reinterpret_cast<iTJSNativeInstance **>(&layer))) ||
           !layer) {
            return nullptr;
        }
        return layer;
    }

    void releaseLayerVariant(tTJSVariant &slot) {
        if(slot.Type() == tvtObject && slot.AsObjectNoAddRef()) {
            iTJSDispatch2 *obj = slot.AsObjectNoAddRef();
            if(auto *layer = resolveNativeLayer(obj)) {
                layer->SetHasImage(false);
            }
            auto closure = slot.AsObjectClosureNoAddRef();
            if(closure.Object) {
                closure.Invalidate(0, nullptr, nullptr, nullptr);
            }
        }
        slot.Clear();
    }

    bool getLayerClassVariant(tTJSVariant &layerClassVar) {
        iTJSDispatch2 *global = TVPGetScriptDispatch();
        if(!global) {
            return false;
        }
        const bool ok =
            TJS_SUCCEEDED(global->PropGet(0, TJS_W("Layer"), nullptr,
                                          &layerClassVar, global)) &&
            layerClassVar.Type() == tvtObject &&
            layerClassVar.AsObjectNoAddRef();
        global->Release();
        return ok;
    }

    iTJSDispatch2 *createLayerObject(const tTJSVariant &owner,
                                     iTJSDispatch2 *parentLayerObject) {
        if(owner.Type() != tvtObject || !owner.AsObjectNoAddRef()) {
            return nullptr;
        }

        tTJSVariant layerClassVar;
        if(!getLayerClassVariant(layerClassVar)) {
            return nullptr;
        }

        iTJSDispatch2 *created = nullptr;
        tTJSVariant ownerArg(owner);
        tTJSVariant parentArg = parentLayerObject
            ? tTJSVariant(parentLayerObject, parentLayerObject)
            : tTJSVariant();
        tTJSVariant *args[] = { &ownerArg, &parentArg };
        if(TJS_FAILED(layerClassVar.AsObjectNoAddRef()->CreateNew(
               0, nullptr, nullptr, &created, 2, args,
               layerClassVar.AsObjectNoAddRef()))) {
            return nullptr;
        }
        return created;
    }

    iTJSDispatch2 *ensureLayerObject(tTJSVariant &slot,
                                     const tTJSVariant &owner,
                                     iTJSDispatch2 *parentLayerObject,
                                     bool visible) {
        iTJSDispatch2 *layerObject =
            slot.Type() == tvtObject ? slot.AsObjectNoAddRef() : nullptr;
        if(!layerObject) {
            layerObject = createLayerObject(owner, parentLayerObject);
            if(!layerObject) {
                return nullptr;
            }
            slot = tTJSVariant(layerObject, layerObject);
            layerObject->Release();
            layerObject = slot.AsObjectNoAddRef();
        }

        auto *layer = resolveNativeLayer(layerObject);
        if(!layer) {
            return nullptr;
        }
        if(parentLayerObject) {
            if(auto *parentLayer = resolveNativeLayer(parentLayerObject);
               parentLayer && layer->GetParent() != parentLayer) {
                layer->SetParent(parentLayer);
            }
        }
        layer->SetType(ltAlpha);
        layer->SetVisible(visible);
        layer->SetAbsoluteOrderMode(false);
        return layerObject;
    }

    bool assignBitmapToLayerLike_0x6948E8(tTJSNI_BaseLayer *sourceLayer,
                                          const iTVPBaseBitmap &src) {
        if(!sourceLayer || src.GetWidth() <= 0 || src.GetHeight() <= 0) {
            return false;
        }
        if(!sourceLayer->GetHasImage()) {
            sourceLayer->SetHasImage(true);
        }
        sourceLayer->SetType(ltAlpha);
        sourceLayer->AssignMainImageWithUpdate(
            const_cast<iTVPBaseBitmap *>(&src));
        sourceLayer->SetSize(src.GetWidth(), src.GetHeight());
        sourceLayer->SetClip(0, 0, src.GetWidth(), src.GetHeight());
        return true;
    }

} // namespace

namespace motion {

    SourceCache::SourceCache() = default;

    SourceCache::SourceCache(tTJSVariant owner, tjs_int layerType) {
        setLayerOwner(std::move(owner), layerType);
    }

    SourceCache::~SourceCache() {
        clearCache();
        releaseLayerVariant(_bufLayer);
    }

    void SourceCache::bindRuntime(detail::PlayerRuntime *runtime,
                                  ResourceManager *resourceManager) {
        _runtime = runtime;
        _resourceManager = resourceManager;
    }

    void SourceCache::setSelfObject(tTJSVariant selfObject) {
        _selfObject = std::move(selfObject);
    }

    void SourceCache::setLayerOwner(tTJSVariant owner, tjs_int layerType) {
        _owner = std::move(owner);
        _layerType = layerType;
        _primaryLayer.Clear();

        if(_owner.Type() == tvtObject && _owner.AsObjectNoAddRef()) {
            tTJSVariant primary;
            if(getObjectProperty(_owner, TJS_W("primaryLayer"), primary) &&
               primary.Type() == tvtObject && primary.AsObjectNoAddRef()) {
                _primaryLayer = primary;
            } else if(resolveNativeLayer(_owner.AsObjectNoAddRef())) {
                _primaryLayer = _owner;
            }
        }

        iTJSDispatch2 *parentLayer = _primaryLayer.Type() == tvtObject
            ? _primaryLayer.AsObjectNoAddRef()
            : nullptr;
        const tTJSVariant &layerOwner = _owner;
        if(layerOwner.Type() == tvtObject) {
            ensureLayerObject(_bufLayer, layerOwner, parentLayer, false);
        }
    }

    tTJSVariant SourceCache::loadSource(tTJSVariant keyOrSource,
                                        tTJSVariant currentSource) {
        auto name = sourceNameFromVariant(keyOrSource);
        if(!name || name->IsEmpty()) {
            name = sourceNameFromVariant(currentSource);
        }
        if(!name || name->IsEmpty()) {
            return {};
        }
        return loadSourceByName(*name, currentSource);
    }

    tTJSVariant
    SourceCache::loadSourceByName(const ttstr &name,
                                  const tTJSVariant &currentSource) {
        const auto key = detail::narrow(name);
        if(key.empty()) {
            return {};
        }

        if(auto *entry = findEntryByKey(key)) {
            if(entry->sourceObject.Type() != tvtVoid) {
                return entry->sourceObject;
            }
            return entry->rawSource;
        }

        std::string resolvedKey;
        auto rawSource = currentSource.Type() != tvtVoid
            ? currentSource
            : loadRawSourceVariant(name, resolvedKey);
        Entry entry;
        entry.key = key;
        entry.resolvedKey = resolvedKey.empty() ? key : resolvedKey;
        entry.rawSource = rawSource;
        _entries.push_front(std::move(entry));
        return rawSource;
    }

    tTJSVariant SourceCache::loadRenderSourceByName(
        const ttstr &name, const tTJSVariant &currentSource, int blendMode,
        const std::array<std::uint32_t, 4> &packedColors,
        iTJSDispatch2 *layerTreeOwnerObject, iTJSDispatch2 *parentLayerObject) {
        const auto key = detail::narrow(name);
        if(key.empty()) {
            return {};
        }

        if(layerTreeOwnerObject &&
           (_owner.Type() != tvtObject || _primaryLayer.Type() != tvtObject)) {
            setLayerOwner(
                tTJSVariant(layerTreeOwnerObject, layerTreeOwnerObject),
                _layerType);
        }
        if(parentLayerObject && _primaryLayer.Type() != tvtObject) {
            _primaryLayer = tTJSVariant(parentLayerObject, parentLayerObject);
        }

        if(auto *entry = findEntry(key, blendMode, packedColors)) {
            if(entry->sourceObject.Type() == tvtObject &&
               entry->sourceObject.AsObjectNoAddRef()) {
                return entry->sourceObject;
            }
        }

        const bool needsTint = packedColorsNeedTint(packedColors);
        std::string resolvedKey;
        tTJSVariant rawSource;
        if(currentSource.Type() != tvtVoid) {
            rawSource = currentSource;
        } else if(key.rfind("src/", 0) != 0) {
            rawSource = loadRawSourceVariant(name, resolvedKey);
        }
        auto &entry = ensureEntry(key, resolvedKey.empty() ? key : resolvedKey,
                                  blendMode, packedColors, needsTint);
        if(rawSource.Type() != tvtVoid) {
            entry.rawSource = rawSource;
        }

        bool contentDirty = false;
        if(!ensureEntryBackingBitmap(entry, key, blendMode, packedColors,
                                     contentDirty)) {
            return entry.rawSource;
        }

        iTJSDispatch2 *parentLayer = parentLayerObject
            ? parentLayerObject
            : (_primaryLayer.Type() == tvtObject
                   ? _primaryLayer.AsObjectNoAddRef()
                   : nullptr);
        const tTJSVariant owner = _owner.Type() == tvtObject
            ? _owner
            : (layerTreeOwnerObject
                   ? tTJSVariant(layerTreeOwnerObject, layerTreeOwnerObject)
                   : tTJSVariant());
        const bool hadLayer = entry.sourceObject.Type() == tvtObject &&
            entry.sourceObject.AsObjectNoAddRef() != nullptr;
        auto *sourceLayerObject =
            ensureLayerObject(entry.sourceObject, owner, parentLayer, false);
        auto *sourceLayer = resolveNativeLayer(sourceLayerObject);
        if(!sourceLayerObject || !sourceLayer || !entry.backingBitmap) {
            releaseLayerVariant(entry.sourceObject);
            return entry.rawSource;
        }
        if(!hadLayer || contentDirty) {
            if(!assignBitmapToLayerLike_0x6948E8(sourceLayer,
                                                 *entry.backingBitmap)) {
                releaseLayerVariant(entry.sourceObject);
                return entry.rawSource;
            }
        }

        evictEntriesIfNeeded();
        return entry.sourceObject;
    }

    iTVPTexture2D *SourceCache::loadRenderSourceTextureByName(
        const ttstr &name, const tTJSVariant &currentSource, int blendMode,
        const std::array<std::uint32_t, 4> &packedColors) {
        const auto key = detail::narrow(name);
        if(key.empty()) {
            return nullptr;
        }

        if(auto *entry = findEntry(key, blendMode, packedColors)) {
            if(entry->sourceTexture) {
                return entry->sourceTexture;
            }
        }

        const bool needsTint = packedColorsNeedTint(packedColors);
        std::string resolvedKey;
        tTJSVariant rawSource;
        if(currentSource.Type() != tvtVoid) {
            rawSource = currentSource;
        } else if(key.rfind("src/", 0) != 0) {
            rawSource = loadRawSourceVariant(name, resolvedKey);
        }
        auto &entry = ensureEntry(key, resolvedKey.empty() ? key : resolvedKey,
                                  blendMode, packedColors, needsTint);
        if(rawSource.Type() != tvtVoid) {
            entry.rawSource = rawSource;
        }

        bool contentDirty = false;
        if(!ensureEntryBackingBitmap(entry, key, blendMode, packedColors,
                                     contentDirty)) {
            return nullptr;
        }
        if(entry.sourceTexture && !contentDirty) {
            return entry.sourceTexture;
        }

        const auto width = entry.backingBitmap->GetWidth();
        const auto height = entry.backingBitmap->GetHeight();
        const auto pitch = entry.backingBitmap->GetPitchBytes();
        const auto *pixels = entry.backingBitmap->GetScanLine(0);
        if(!pixels || pitch <= 0 || width <= 0 || height <= 0) {
            return nullptr;
        }
        const auto fmt = entry.backingBitmap->Is8BPP() ? TVPTextureFormat::Gray
                                                       : TVPTextureFormat::RGBA;
        if(entry.sourceTexture) {
            if(entry.sourceTexture->CanUpdateInPlace() &&
               entry.sourceTexture->IsIndependent() &&
               entry.sourceTexture->GetWidth() == width &&
               entry.sourceTexture->GetHeight() == height &&
               entry.sourceTexture->GetFormat() == fmt) {
                entry.sourceTexture->Update(pixels, fmt, pitch,
                                            tTVPRect(0, 0, width, height));
                evictEntriesIfNeeded();
                return entry.sourceTexture;
            }
            releaseEntryTexture(entry);
        }

        // D3DAdaptor_renderFromPlayer @ 0x6ADE24 passes a source texture
        // getter into 0x6ADFBC, so this path returns texture data directly
        // instead of materializing an intermediate SourceCache Layer.
        entry.sourceTexture = TVPGetRenderManager()->CreateTexture2D(
            pixels, pitch, width, height, fmt,
            RENDER_CREATE_TEXTURE_FLAG_NO_COMPRESS);
        evictEntriesIfNeeded();
        return entry.sourceTexture;
    }

    tTJSVariant SourceCache::findSource(ttstr name) {
        return loadSourceByName(name, {});
    }

    void SourceCache::clearCache() {
        for(auto &entry : _entries) {
            releaseEntryResources(entry);
        }
        _entries.clear();
        _totalEntryBytes = 0;
        _baseBitmaps.clear();
        _totalBaseBitmapBytes = 0;
        if(_runtime && _runtime->activeMotion) {
            _runtime->activeMotion->decodedAtlasLru.clear();
            _runtime->activeMotion->decodedAtlasBytes = 0;
        }
    }

    void SourceCache::eraseSource(ttstr name) {
        const auto key = detail::narrow(name);
        if(key.empty()) {
            return;
        }

        for(auto it = _entries.begin(); it != _entries.end();) {
            if(it->key == key || it->resolvedKey == key) {
                releaseEntryResources(*it);
                it = _entries.erase(it);
            } else {
                ++it;
            }
        }
        for(auto it = _baseBitmaps.begin(); it != _baseBitmaps.end();) {
            if(it->key == key) {
                _totalBaseBitmapBytes -= it->bytes;
                it = _baseBitmaps.erase(it);
            } else {
                ++it;
            }
        }
    }

    tTJSVariant SourceCache::getBufLayer() const { return _bufLayer; }

    std::size_t SourceCache::size() const { return _entries.size(); }

    const SourceCache::Entry *SourceCache::findEntry(
        const std::string &key, int blendMode,
        const std::array<std::uint32_t, 4> &packedColors) const {
        for(const auto &entry : _entries) {
            if((entry.key == key || entry.resolvedKey == key) &&
               entry.blendMode == blendMode &&
               entry.packedColors == packedColors) {
                return &entry;
            }
        }
        return nullptr;
    }

    SourceCache::Entry *
    SourceCache::findEntry(const std::string &key, int blendMode,
                           const std::array<std::uint32_t, 4> &packedColors) {
        for(auto it = _entries.begin(); it != _entries.end(); ++it) {
            if((it->key == key || it->resolvedKey == key) &&
               it->blendMode == blendMode && it->packedColors == packedColors) {
                _entries.splice(_entries.begin(), _entries, it);
                return &_entries.front();
            }
        }
        return nullptr;
    }

    SourceCache::Entry *SourceCache::findTintedEntry(const std::string &key,
                                                     int blendMode) {
        for(auto it = _entries.begin(); it != _entries.end(); ++it) {
            if(it->isTinted && (it->key == key || it->resolvedKey == key) &&
               it->blendMode == blendMode) {
                _entries.splice(_entries.begin(), _entries, it);
                return &_entries.front();
            }
        }
        return nullptr;
    }

    SourceCache::Entry *SourceCache::findEntryByKey(const std::string &key) {
        for(auto it = _entries.begin(); it != _entries.end(); ++it) {
            if(it->key == key || it->resolvedKey == key) {
                _entries.splice(_entries.begin(), _entries, it);
                return &_entries.front();
            }
        }
        return nullptr;
    }

    SourceCache::Entry &SourceCache::ensureEntry(
        const std::string &key, const std::string &resolvedKey, int blendMode,
        const std::array<std::uint32_t, 4> &packedColors, bool isTinted) {
        if(auto *entry = findEntry(key, blendMode, packedColors)) {
            return *entry;
        }
        if(isTinted) {
            if(auto *reusable = findTintedEntry(key, blendMode)) {
                return *reusable;
            }
        }

        Entry entry;
        entry.key = key;
        entry.resolvedKey = resolvedKey.empty() ? key : resolvedKey;
        entry.blendMode = blendMode;
        entry.isTinted = isTinted;
        entry.packedColors = packedColors;
        _entries.push_front(std::move(entry));
        return _entries.front();
    }

    std::shared_ptr<tTVPBaseBitmap>
    SourceCache::loadOrGetBaseBitmap(const std::string &key) {
        for(auto it = _baseBitmaps.begin(); it != _baseBitmaps.end(); ++it) {
            if(it->key == key) {
                _baseBitmaps.splice(_baseBitmaps.begin(), _baseBitmaps, it);
                return _baseBitmaps.front().bitmap;
            }
        }

        std::shared_ptr<tTVPBaseBitmap> baseBitmap;
        if(_runtime && _runtime->activeMotion) {
            const auto &snap = *_runtime->activeMotion;
            const bool preferEmbeddedPsb = key.rfind("src/", 0) == 0;
            if(preferEmbeddedPsb) {
                baseBitmap = loadPsbBitmap(snap, key);
                if(!baseBitmap) {
                    for(const auto &attached : snap.attachedSnapshots) {
                        if(attached) {
                            baseBitmap = loadPsbBitmap(*attached, key);
                            if(baseBitmap) {
                                break;
                            }
                        }
                    }
                }
            }
            if(!baseBitmap) {
                const auto path =
                    resolveMotionSourcePathLike_0x6948E8(snap, key);
                baseBitmap = loadGraphicBitmap(path);
            }
            if(!baseBitmap && !preferEmbeddedPsb) {
                baseBitmap = loadPsbBitmap(snap, key);
                if(!baseBitmap) {
                    for(const auto &attached : snap.attachedSnapshots) {
                        if(attached) {
                            baseBitmap = loadPsbBitmap(*attached, key);
                            if(baseBitmap) {
                                break;
                            }
                        }
                    }
                }
            }
        }
        if(!baseBitmap || baseBitmap->GetWidth() <= 0 ||
           baseBitmap->GetHeight() <= 0) {
            return nullptr;
        }

        const std::size_t bytes = bitmapByteSize(baseBitmap.get());
        _baseBitmaps.push_front({ key, baseBitmap, bytes });
        _totalBaseBitmapBytes += bytes;
        while(_baseBitmaps.size() > kMaxBaseBitmapEntries ||
              (_baseBitmaps.size() > 1 &&
               _totalBaseBitmapBytes > kMaxBaseBitmapBytes)) {
            _totalBaseBitmapBytes -= _baseBitmaps.back().bytes;
            _baseBitmaps.pop_back();
        }
        return baseBitmap;
    }

    bool SourceCache::ensureEntryBackingBitmap(
        Entry &entry, const std::string &key, int blendMode,
        const std::array<std::uint32_t, 4> &packedColors, bool &contentDirty) {
        contentDirty = false;
        const bool needsTint = packedColorsNeedTint(packedColors);
        if(entry.backingBitmap && entry.packedColors == packedColors &&
           entry.isTinted == needsTint) {
            return entry.backingBitmap->GetWidth() > 0 &&
                entry.backingBitmap->GetHeight() > 0;
        }

        auto baseBitmap = loadOrGetBaseBitmap(key);
        if(!baseBitmap || baseBitmap->GetWidth() <= 0 ||
           baseBitmap->GetHeight() <= 0) {
            return false;
        }

        const bool useHalfAlphaTint = (blendMode & 0xF0) == 0x10;
        if(needsTint) {
            const tjs_uint w = baseBitmap->GetWidth();
            const tjs_uint h = baseBitmap->GetHeight();
            if(!entry.backingBitmap || entry.backingBitmap == baseBitmap ||
               entry.backingBitmap->GetWidth() != w ||
               entry.backingBitmap->GetHeight() != h) {
                entry.backingBitmap =
                    std::make_shared<tTVPBaseBitmap>(w, h, 32);
            }
            applyPackedCornerTintFromBaseLike_0x6A7518(
                *baseBitmap, *entry.backingBitmap, packedColors,
                useHalfAlphaTint);
        } else {
            entry.backingBitmap = baseBitmap;
        }

        entry.packedColors = packedColors;
        entry.isTinted = needsTint;
        const std::size_t newBytes = bitmapByteSize(entry.backingBitmap.get());
        _totalEntryBytes -= entry.imageBytes;
        entry.imageBytes = newBytes;
        _totalEntryBytes += entry.imageBytes;
        contentDirty = true;
        return true;
    }

    void SourceCache::releaseEntryTexture(Entry &entry) {
        if(entry.sourceTexture) {
            entry.sourceTexture->Release();
            entry.sourceTexture = nullptr;
        }
    }

    void SourceCache::releaseEntryResources(Entry &entry) {
        releaseEntryTexture(entry);
        releaseLayerVariant(entry.sourceObject);
        entry.backingBitmap.reset();
        _totalEntryBytes -= entry.imageBytes;
        entry.imageBytes = 0;
    }

    void SourceCache::evictEntriesIfNeeded() {
        while(_entries.size() > kMaxSourceCacheEntries ||
              (_entries.size() > 1 &&
               _totalEntryBytes > kMaxSourceCacheEntryBytes)) {
            releaseEntryResources(_entries.back());
            _entries.pop_back();
        }
    }

    tTJSVariant
    SourceCache::loadRawSourceVariant(const ttstr &name,
                                      std::string &resolvedKey) const {
        resolvedKey.clear();
        if(!_runtime || !_resourceManager) {
            return {};
        }

        ttstr resolved;
        if(!detail::resolveExistingPath(
               internal::buildSourceCandidates(*_runtime, name), resolved)) {
            return {};
        }

        resolvedKey = detail::narrow(resolved);
        return _resourceManager->load(resolved);
    }

} // namespace motion
