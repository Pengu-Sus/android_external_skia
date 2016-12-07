/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkCanvas.h"
#include "SkSVGCircle.h"
#include "SkSVGRenderContext.h"
#include "SkSVGValue.h"

SkSVGCircle::SkSVGCircle() : INHERITED(SkSVGTag::kCircle) {}

void SkSVGCircle::setCx(const SkSVGLength& cx) {
    fCx = cx;
}

void SkSVGCircle::setCy(const SkSVGLength& cy) {
    fCy = cy;
}

void SkSVGCircle::setR(const SkSVGLength& r) {
    fR = r;
}

void SkSVGCircle::onSetAttribute(SkSVGAttribute attr, const SkSVGValue& v) {
    switch (attr) {
    case SkSVGAttribute::kCx:
        if (const auto* cx = v.as<SkSVGLengthValue>()) {
            this->setCx(*cx);
        }
        break;
    case SkSVGAttribute::kCy:
        if (const auto* cy = v.as<SkSVGLengthValue>()) {
            this->setCy(*cy);
        }
        break;
    case SkSVGAttribute::kR:
        if (const auto* r = v.as<SkSVGLengthValue>()) {
            this->setR(*r);
        }
        break;
    default:
        this->INHERITED::onSetAttribute(attr, v);
    }
}

void SkSVGCircle::onDraw(SkCanvas* canvas, const SkSVGLengthContext& lctx,
                         const SkPaint& paint, SkPath::FillType) const {
    const auto cx = lctx.resolve(fCx, SkSVGLengthContext::LengthType::kHorizontal);
    const auto cy = lctx.resolve(fCy, SkSVGLengthContext::LengthType::kVertical);
    const auto  r = lctx.resolve(fR , SkSVGLengthContext::LengthType::kOther);

    if (r > 0) {
        canvas->drawCircle(cx, cy, r, paint);
    }
}
