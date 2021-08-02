/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/GrDirectContextPriv.h"

#include "include/gpu/GrContextThreadSafeProxy.h"
#include "include/gpu/GrDirectContext.h"
#include "src/core/SkRuntimeEffectPriv.h"
#include "src/gpu/GrContextThreadSafeProxyPriv.h"
#include "src/gpu/GrDrawingManager.h"
#include "src/gpu/GrGpu.h"
#include "src/gpu/GrMemoryPool.h"
#include "src/gpu/GrRecordingContextPriv.h"
#include "src/gpu/GrSurfaceContext.h"
#include "src/gpu/GrSurfaceFillContext.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/GrThreadSafePipelineBuilder.h"
#include "src/gpu/SkGr.h"
#include "src/gpu/effects/GrSkSLFP.h"
#include "src/gpu/effects/GrTextureEffect.h"
#include "src/gpu/text/GrAtlasManager.h"
#include "src/gpu/text/GrTextBlobCache.h"
#include "src/image/SkImage_Base.h"
#include "src/image/SkImage_Gpu.h"

#define ASSERT_OWNED_PROXY(P) \
    SkASSERT(!(P) || !((P)->peekTexture()) || (P)->peekTexture()->getContext() == fContext)
#define ASSERT_SINGLE_OWNER GR_ASSERT_SINGLE_OWNER(fContext->singleOwner())
#define RETURN_VALUE_IF_ABANDONED(value) if (fContext->abandoned()) { return (value); }

sk_sp<const GrCaps> GrDirectContextPriv::refCaps() const {
    return fContext->refCaps();
}

void GrDirectContextPriv::addOnFlushCallbackObject(GrOnFlushCallbackObject* onFlushCBObject) {
    fContext->addOnFlushCallbackObject(onFlushCBObject);
}

GrSemaphoresSubmitted GrDirectContextPriv::flushSurfaces(
                                                    SkSpan<GrSurfaceProxy*> proxies,
                                                    SkSurface::BackendSurfaceAccess access,
                                                    const GrFlushInfo& info,
                                                    const GrBackendSurfaceMutableState* newState) {
    ASSERT_SINGLE_OWNER
    GR_CREATE_TRACE_MARKER_CONTEXT("GrDirectContextPriv", "flushSurfaces", fContext);

    if (fContext->abandoned()) {
        if (info.fSubmittedProc) {
            info.fSubmittedProc(info.fSubmittedContext, false);
        }
        if (info.fFinishedProc) {
            info.fFinishedProc(info.fFinishedContext);
        }
        return GrSemaphoresSubmitted::kNo;
    }

#ifdef SK_DEBUG
    for (GrSurfaceProxy* proxy : proxies) {
        SkASSERT(proxy);
        ASSERT_OWNED_PROXY(proxy);
    }
#endif
    return fContext->drawingManager()->flushSurfaces(proxies, access, info, newState);
}

void GrDirectContextPriv::createDDLTask(sk_sp<const SkDeferredDisplayList> ddl,
                                        sk_sp<GrRenderTargetProxy> newDest,
                                        SkIPoint offset) {
    fContext->drawingManager()->createDDLTask(std::move(ddl), std::move(newDest), offset);
}

bool GrDirectContextPriv::compile(const GrProgramDesc& desc, const GrProgramInfo& info) {
    GrGpu* gpu = this->getGpu();
    if (!gpu) {
        return false;
    }

    return gpu->compile(desc, info);
}


//////////////////////////////////////////////////////////////////////////////
#if GR_TEST_UTILS

void GrDirectContextPriv::dumpCacheStats(SkString* out) const {
#if GR_CACHE_STATS
    fContext->fResourceCache->dumpStats(out);
#endif
}

void GrDirectContextPriv::dumpCacheStatsKeyValuePairs(SkTArray<SkString>* keys,
                                                      SkTArray<double>* values) const {
#if GR_CACHE_STATS
    fContext->fResourceCache->dumpStatsKeyValuePairs(keys, values);
#endif
}

void GrDirectContextPriv::printCacheStats() const {
    SkString out;
    this->dumpCacheStats(&out);
    SkDebugf("%s", out.c_str());
}

/////////////////////////////////////////////////
void GrDirectContextPriv::resetGpuStats() const {
#if GR_GPU_STATS
    fContext->fGpu->stats()->reset();
#endif
}

void GrDirectContextPriv::dumpGpuStats(SkString* out) const {
#if GR_GPU_STATS
    fContext->fGpu->stats()->dump(out);
    if (auto builder = fContext->fGpu->pipelineBuilder()) {
        builder->stats()->dump(out);
    }
#endif
}

void GrDirectContextPriv::dumpGpuStatsKeyValuePairs(SkTArray<SkString>* keys,
                                                    SkTArray<double>* values) const {
#if GR_GPU_STATS
    fContext->fGpu->stats()->dumpKeyValuePairs(keys, values);
    if (auto builder = fContext->fGpu->pipelineBuilder()) {
        builder->stats()->dumpKeyValuePairs(keys, values);
    }
#endif
}

void GrDirectContextPriv::printGpuStats() const {
    SkString out;
    this->dumpGpuStats(&out);
    SkDebugf("%s", out.c_str());
}

/////////////////////////////////////////////////
void GrDirectContextPriv::resetContextStats() const {
#if GR_GPU_STATS
    fContext->stats()->reset();
#endif
}

void GrDirectContextPriv::dumpContextStats(SkString* out) const {
#if GR_GPU_STATS
    return fContext->stats()->dump(out);
#endif
}

void GrDirectContextPriv::dumpContextStatsKeyValuePairs(SkTArray<SkString>* keys,
                                                        SkTArray<double>* values) const {
#if GR_GPU_STATS
    return fContext->stats()->dumpKeyValuePairs(keys, values);
#endif
}

void GrDirectContextPriv::printContextStats() const {
    SkString out;
    this->dumpContextStats(&out);
    SkDebugf("%s", out.c_str());
}

/////////////////////////////////////////////////
sk_sp<SkImage> GrDirectContextPriv::testingOnly_getFontAtlasImage(GrMaskFormat format,
                                                                  unsigned int index) {
    auto atlasManager = this->getAtlasManager();
    if (!atlasManager) {
        return nullptr;
    }

    unsigned int numActiveProxies;
    const GrSurfaceProxyView* views = atlasManager->getViews(format, &numActiveProxies);
    if (index >= numActiveProxies || !views || !views[index].proxy()) {
        return nullptr;
    }

    SkColorType colorType = GrColorTypeToSkColorType(GrMaskFormatToColorType(format));
    SkASSERT(views[index].proxy()->priv().isExact());
    return sk_make_sp<SkImage_Gpu>(sk_ref_sp(fContext),
                                   kNeedNewImageUniqueID,
                                   views[index],
                                   SkColorInfo(colorType, kPremul_SkAlphaType, nullptr));
}

void GrDirectContextPriv::testingOnly_flushAndRemoveOnFlushCallbackObject(
        GrOnFlushCallbackObject* cb) {
    fContext->flushAndSubmit();
    fContext->drawingManager()->testingOnly_removeOnFlushCallbackObject(cb);
}
#endif

// Both of these effects aggressively round to the nearest exact (N / 255) floating point values.
// This lets us find a round-trip preserving pair on some GPUs that do odd byte to float conversion.
static std::unique_ptr<GrFragmentProcessor> make_premul_effect(
        std::unique_ptr<GrFragmentProcessor> fp) {
    if (!fp) {
        return nullptr;
    }

    static auto effect = SkMakeRuntimeEffect(SkRuntimeEffect::MakeForColorFilter, R"(
        half4 main(half4 color) {
            color = floor(color * 255 + 0.5) / 255;
            color.rgb = floor(color.rgb * color.a * 255 + 0.5) / 255;
            return color;
        }
    )");

    fp = GrSkSLFP::Make(effect, "ToPremul", std::move(fp), GrSkSLFP::OptFlags::kNone);
    return GrFragmentProcessor::HighPrecision(std::move(fp));
}

static std::unique_ptr<GrFragmentProcessor> make_unpremul_effect(
        std::unique_ptr<GrFragmentProcessor> fp) {
    if (!fp) {
        return nullptr;
    }

    static auto effect = SkMakeRuntimeEffect(SkRuntimeEffect::MakeForColorFilter, R"(
        half4 main(half4 color) {
            color = floor(color * 255 + 0.5) / 255;
            color.rgb = color.a <= 0 ? half3(0) : floor(color.rgb / color.a * 255 + 0.5) / 255;
            return color;
        }
    )");

    fp = GrSkSLFP::Make(effect, "ToUnpremul", std::move(fp), GrSkSLFP::OptFlags::kNone);
    return GrFragmentProcessor::HighPrecision(std::move(fp));
}

static bool test_for_preserving_PM_conversions(GrDirectContext* dContext) {
    static constexpr int kSize = 256;
    SkAutoTMalloc<uint32_t> data(kSize * kSize * 3);
    uint32_t* srcData = data.get();

    // Fill with every possible premultiplied A, color channel value. There will be 256-y duplicate
    // values in row y. We set r, g, and b to the same value since they are handled identically.
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            uint8_t* color = reinterpret_cast<uint8_t*>(&srcData[kSize*y + x]);
            color[3] = y;
            color[2] = std::min(x, y);
            color[1] = std::min(x, y);
            color[0] = std::min(x, y);
        }
    }

    const SkImageInfo pmII =
            SkImageInfo::Make(kSize, kSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    const SkImageInfo upmII = pmII.makeAlphaType(kUnpremul_SkAlphaType);

    auto readSFC = dContext->priv().makeSFC(upmII, SkBackingFit::kExact);
    auto tempSFC = dContext->priv().makeSFC(pmII,  SkBackingFit::kExact);
    if (!readSFC || !tempSFC) {
        return false;
    }

    // This function is only ever called if we are in a GrDirectContext since we are calling read
    // pixels here. Thus the pixel data will be uploaded immediately and we don't need to keep the
    // pixel data alive in the proxy. Therefore the ReleaseProc is nullptr.
    SkBitmap bitmap;
    bitmap.installPixels(pmII, srcData, 4 * kSize);
    bitmap.setImmutable();

    auto dataView = std::get<0>(GrMakeUncachedBitmapProxyView(dContext, bitmap));
    if (!dataView) {
        return false;
    }

    uint32_t* firstRead  = data.get() +   kSize*kSize;
    uint32_t* secondRead = data.get() + 2*kSize*kSize;
    std::fill_n( firstRead, kSize*kSize, 0);
    std::fill_n(secondRead, kSize*kSize, 0);

    GrPixmap firstReadPM( upmII,  firstRead, kSize*sizeof(uint32_t));
    GrPixmap secondReadPM(upmII, secondRead, kSize*sizeof(uint32_t));

    // We do a PM->UPM draw from dataTex to readTex and read the data. Then we do a UPM->PM draw
    // from readTex to tempTex followed by a PM->UPM draw to readTex and finally read the data.
    // We then verify that two reads produced the same values.

    auto fp1 = make_unpremul_effect(GrTextureEffect::Make(std::move(dataView), bitmap.alphaType()));
    readSFC->fillRectWithFP(SkIRect::MakeWH(kSize, kSize), std::move(fp1));
    if (!readSFC->readPixels(dContext, firstReadPM, {0, 0})) {
        return false;
    }

    auto fp2 = make_premul_effect(
            GrTextureEffect::Make(readSFC->readSurfaceView(), readSFC->colorInfo().alphaType()));
    tempSFC->fillRectWithFP(SkIRect::MakeWH(kSize, kSize), std::move(fp2));

    auto fp3 = make_unpremul_effect(
            GrTextureEffect::Make(tempSFC->readSurfaceView(), tempSFC->colorInfo().alphaType()));
    readSFC->fillRectWithFP(SkIRect::MakeWH(kSize, kSize), std::move(fp3));

    if (!readSFC->readPixels(dContext, secondReadPM, {0, 0})) {
        return false;
    }

    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x <= y; ++x) {
            if (firstRead[kSize*y + x] != secondRead[kSize*y + x]) {
                return false;
            }
        }
    }

    return true;
}

bool GrDirectContextPriv::validPMUPMConversionExists() {
    ASSERT_SINGLE_OWNER

    if (!fContext->fDidTestPMConversions) {
        fContext->fPMUPMConversionsRoundTrip = test_for_preserving_PM_conversions(fContext);
        fContext->fDidTestPMConversions = true;
    }

    // The PM<->UPM tests fail or succeed together so we only need to check one.
    return fContext->fPMUPMConversionsRoundTrip;
}

std::unique_ptr<GrFragmentProcessor> GrDirectContextPriv::createPMToUPMEffect(
        std::unique_ptr<GrFragmentProcessor> fp) {
    ASSERT_SINGLE_OWNER
    // We should have already called this->priv().validPMUPMConversionExists() in this case
    SkASSERT(fContext->fDidTestPMConversions);
    // ...and it should have succeeded
    SkASSERT(this->validPMUPMConversionExists());

    return make_unpremul_effect(std::move(fp));
}

std::unique_ptr<GrFragmentProcessor> GrDirectContextPriv::createUPMToPMEffect(
        std::unique_ptr<GrFragmentProcessor> fp) {
    ASSERT_SINGLE_OWNER
    // We should have already called this->priv().validPMUPMConversionExists() in this case
    SkASSERT(fContext->fDidTestPMConversions);
    // ...and it should have succeeded
    SkASSERT(this->validPMUPMConversionExists());

    return make_premul_effect(std::move(fp));
}

sk_sp<skgpu::BaseDevice> GrDirectContextPriv::createDevice(GrColorType colorType,
                                                           sk_sp<GrSurfaceProxy> proxy,
                                                           sk_sp<SkColorSpace> colorSpace,
                                                           GrSurfaceOrigin origin,
                                                           const SkSurfaceProps& props,
                                                           skgpu::BaseDevice::InitContents init) {
    return fContext->GrRecordingContext::priv().createDevice(colorType, std::move(proxy),
                                                             std::move(colorSpace),
                                                             origin, props, init);
}

sk_sp<skgpu::BaseDevice> GrDirectContextPriv::createDevice(SkBudgeted budgeted,
                                                           const SkImageInfo& ii,
                                                           SkBackingFit fit,
                                                           int sampleCount,
                                                           GrMipmapped mipmapped,
                                                           GrProtected isProtected,
                                                           GrSurfaceOrigin origin,
                                                           const SkSurfaceProps& props,
                                                           skgpu::BaseDevice::InitContents init) {
    return fContext->GrRecordingContext::priv().createDevice(budgeted, ii, fit, sampleCount,
                                                             mipmapped, isProtected,
                                                             origin, props, init);
}

std::unique_ptr<GrSurfaceContext> GrDirectContextPriv::makeSC(GrSurfaceProxyView readView,
                                                              const GrColorInfo& info) {
    return fContext->GrRecordingContext::priv().makeSC(readView, info);
}

std::unique_ptr<GrSurfaceFillContext> GrDirectContextPriv::makeSFC(GrImageInfo info,
                                                                   SkBackingFit fit,
                                                                   int sampleCount,
                                                                   GrMipmapped mipmapped,
                                                                   GrProtected isProtected,
                                                                   GrSurfaceOrigin origin,
                                                                   SkBudgeted budgeted) {
    return fContext->GrRecordingContext::priv().makeSFC(info, fit, sampleCount, mipmapped,
                                                        isProtected, origin, budgeted);
}

std::unique_ptr<GrSurfaceFillContext> GrDirectContextPriv::makeSFCWithFallback(
        GrImageInfo info,
        SkBackingFit fit,
        int sampleCount,
        GrMipmapped mipmapped,
        GrProtected isProtected,
        GrSurfaceOrigin origin,
        SkBudgeted budgeted) {
    return fContext->GrRecordingContext::priv().makeSFCWithFallback(info, fit, sampleCount,
                                                                    mipmapped, isProtected,
                                                                    origin, budgeted);
}

std::unique_ptr<GrSurfaceFillContext> GrDirectContextPriv::makeSFCFromBackendTexture(
        GrColorInfo info,
        const GrBackendTexture& tex,
        int sampleCount,
        GrSurfaceOrigin origin,
        sk_sp<GrRefCntedCallback> releaseHelper) {
    return fContext->GrRecordingContext::priv().makeSFCFromBackendTexture(info, tex, sampleCount,
                                                                          origin,
                                                                          std::move(releaseHelper));

}
