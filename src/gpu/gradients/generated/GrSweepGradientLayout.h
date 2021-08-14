/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrSweepGradientLayout.fp; do not modify.
 **************************************************************************************************/
#ifndef GrSweepGradientLayout_DEFINED
#define GrSweepGradientLayout_DEFINED

#include "include/core/SkM44.h"
#include "include/core/SkTypes.h"

#include "src/gpu/effects/GrMatrixEffect.h"
#include "src/gpu/gradients/GrGradientShader.h"
#include "src/shaders/gradients/SkSweepGradient.h"

#include "src/gpu/GrFragmentProcessor.h"

class GrSweepGradientLayout : public GrFragmentProcessor {
public:
    static std::unique_ptr<GrFragmentProcessor> Make(const SkSweepGradient& gradient,
                                                     const GrFPArgs& args);
    GrSweepGradientLayout(const GrSweepGradientLayout& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "SweepGradientLayout"; }
    float bias;
    float scale;

private:
    GrSweepGradientLayout(float bias, float scale)
            : INHERITED(kGrSweepGradientLayout_ClassID,
                        (OptimizationFlags)kPreservesOpaqueInput_OptimizationFlag)
            , bias(bias)
            , scale(scale) {
        this->setUsesSampleCoordsDirectly();
    }
    std::unique_ptr<GrGLSLFragmentProcessor> onMakeProgramImpl() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
#if GR_TEST_UTILS
    SkString onDumpInfo() const override;
#endif
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    using INHERITED = GrFragmentProcessor;
};
#endif
