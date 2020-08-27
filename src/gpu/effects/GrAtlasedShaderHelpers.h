/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrAtlasedShaderHelpers_DEFINED
#define GrAtlasedShaderHelpers_DEFINED

#include "src/gpu/GrDrawOpAtlas.h"
#include "src/gpu/GrShaderCaps.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLPrimitiveProcessor.h"
#include "src/gpu/glsl/GrGLSLVarying.h"
#include "src/gpu/glsl/GrGLSLVertexGeoBuilder.h"

static void append_index_uv_varyings(GrGLSLPrimitiveProcessor::EmitArgs& args,
                                     int numTextureSamplers,
                                     const char* inTexCoordsName,
                                     const char* atlasDimensionsInvName,
                                     GrGLSLVarying* uv,
                                     GrGLSLVarying* texIdx,
                                     GrGLSLVarying* st) {
    using Interpolation = GrGLSLVaryingHandler::Interpolation;
    constexpr int bitShift = GrDrawOpAtlas::kPageIndexBit;
    // This extracts the texture index and texel coordinates from the same variable
    // Packing structure: texel coordinates have the 2-bit texture page encoded in bit 13 of uv,
    // with the high bit of the page in u, and to low bit in v.
    if (args.fShaderCaps->integerSupport()) {
        args.fVertBuilder->codeAppendf("int2 coords = int2(%s.x, %s.y);",
                                       inTexCoordsName, inTexCoordsName);
        constexpr int mask = (1 << bitShift) - 1;
        args.fVertBuilder->codeAppendf(
                "float2 unormTexCoords = float2(coords.x & %d, coords.y & %d);", mask, mask);
        if (numTextureSamplers <= 1) {
            args.fVertBuilder->codeAppend("int texIdx = 0;");
        } else {
            args.fVertBuilder->codeAppendf(
                    "int texIdx = ((coords.x >> %d) & 0x2) + ((coords.y >> %d) & 0x1);",
                    bitShift - 1, bitShift);
        }
    } else {
        args.fVertBuilder->defineConstantf("float", "kIndexBit", "exp2(%d.0)", bitShift);
        args.fVertBuilder->defineConstantf("float", "kInvIndexBit", "exp2(-%d.0)", bitShift);
        args.fVertBuilder->codeAppendf("float2 coord = float2(%s.x, %s.y);",
                                       inTexCoordsName, inTexCoordsName);
        args.fVertBuilder->codeAppend("float2 unitTexCoords = kInvIndexBit * coord;");
        args.fVertBuilder->codeAppend("float2 highBits = floor(unitTexCoords);");
        args.fVertBuilder->codeAppend(
                "float2 unormTexCoords = coord - kIndexBit * highBits;");
        if (numTextureSamplers <= 1) {
            args.fVertBuilder->codeAppend("float texIdx = 0;");
        } else {
            args.fVertBuilder->codeAppend("float texIdx = highBits.x * 2.0 + highBits.y;");
        }
    }

    // Multiply by 1/atlasDimensions to get normalized texture coordinates
    args.fVaryingHandler->addVarying("TextureCoords", uv);
    args.fVertBuilder->codeAppendf(
            "%s = unormTexCoords * %s;", uv->vsOut(), atlasDimensionsInvName);

    args.fVaryingHandler->addVarying("TexIndex", texIdx, args.fShaderCaps->integerSupport()
                                                                 ? Interpolation::kMustBeFlat
                                                                 : Interpolation::kCanBeFlat);
    args.fVertBuilder->codeAppendf("%s = texIdx;", texIdx->vsOut());

    if (st) {
        args.fVaryingHandler->addVarying("IntTextureCoords", st);
        args.fVertBuilder->codeAppendf("%s = unormTexCoords;", st->vsOut());
    }
}

static void append_multitexture_lookup(GrGLSLPrimitiveProcessor::EmitArgs& args,
                                       int numTextureSamplers,
                                       const GrGLSLVarying &texIdx,
                                       const char* coordName,
                                       const char* colorName) {
    SkASSERT(numTextureSamplers > 0);
    // This shouldn't happen, but will avoid a crash if it does
    if (numTextureSamplers <= 0) {
        args.fFragBuilder->codeAppendf("%s = float4(1, 1, 1, 1);", colorName);
        return;
    }

    // conditionally load from the indexed texture sampler
    for (int i = 0; i < numTextureSamplers-1; ++i) {
        args.fFragBuilder->codeAppendf("if (%s == %d) { %s = ", texIdx.fsIn(), i, colorName);
        args.fFragBuilder->appendTextureLookup(args.fTexSamplers[i], coordName);
        args.fFragBuilder->codeAppend("; } else ");
    }
    args.fFragBuilder->codeAppendf("{ %s = ", colorName);
    args.fFragBuilder->appendTextureLookup(args.fTexSamplers[numTextureSamplers - 1], coordName);
    args.fFragBuilder->codeAppend("; }");
}

#endif
