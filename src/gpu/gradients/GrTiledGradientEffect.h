/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrTiledGradientEffect.fp; do not modify.
 **************************************************************************************************/
#ifndef GrTiledGradientEffect_DEFINED
#define GrTiledGradientEffect_DEFINED
#include "SkTypes.h"
#include "GrFragmentProcessor.h"
#include "GrCoordTransform.h"
class GrTiledGradientEffect : public GrFragmentProcessor {
public:
    int colorizer_index() const { return fColorizer_index; }
    int gradLayout_index() const { return fGradLayout_index; }
    bool mirror() const { return fMirror; }
    bool makePremul() const { return fMakePremul; }
    bool colorsAreOpaque() const { return fColorsAreOpaque; }
    static std::unique_ptr<GrFragmentProcessor> Make(
            std::unique_ptr<GrFragmentProcessor> colorizer,
            std::unique_ptr<GrFragmentProcessor> gradLayout, bool mirror, bool makePremul,
            bool colorsAreOpaque) {
        return std::unique_ptr<GrFragmentProcessor>(new GrTiledGradientEffect(
                std::move(colorizer), std::move(gradLayout), mirror, makePremul, colorsAreOpaque));
    }
    GrTiledGradientEffect(const GrTiledGradientEffect& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "TiledGradientEffect"; }

private:
    GrTiledGradientEffect(std::unique_ptr<GrFragmentProcessor> colorizer,
                          std::unique_ptr<GrFragmentProcessor> gradLayout, bool mirror,
                          bool makePremul, bool colorsAreOpaque)
            : INHERITED(kGrTiledGradientEffect_ClassID,
                        (OptimizationFlags)kCompatibleWithCoverageAsAlpha_OptimizationFlag |
                                (colorsAreOpaque && gradLayout->preservesOpaqueInput()
                                         ? kPreservesOpaqueInput_OptimizationFlag
                                         : kNone_OptimizationFlags))
            , fMirror(mirror)
            , fMakePremul(makePremul)
            , fColorsAreOpaque(colorsAreOpaque) {
        SkASSERT(colorizer);
        fColorizer_index = this->numChildProcessors();
        this->registerChildProcessor(std::move(colorizer));
        SkASSERT(gradLayout);
        fGradLayout_index = this->numChildProcessors();
        this->registerChildProcessor(std::move(gradLayout));
    }
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    int fColorizer_index = -1;
    int fGradLayout_index = -1;
    bool fMirror;
    bool fMakePremul;
    bool fColorsAreOpaque;
    typedef GrFragmentProcessor INHERITED;
};
#endif
