/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkRive_Streamreader_DEFINED
#define SkRive_Streamreader_DEFINED

#include <memory>

#include "include/core/SkRefCnt.h"

class SkStreamAsset;
class SkString;

namespace skrive::internal {

class StreamReader {
public:
    virtual ~StreamReader() = default;

    static std::unique_ptr<StreamReader> Make(std::unique_ptr<SkStreamAsset>);
    static std::unique_ptr<StreamReader> Make(const char[], size_t);

    enum class BlockType : uint8_t {
        kUnknown       =   0,
        kComponents    =   1,
        kActorNode     =   2,

        kActorArtboard = 114,
        kArtboards     = 115,

        // End-of-block marker
        kEoB           = 0xff,
    };

    virtual BlockType openBlock() = 0;
    virtual void     closeBlock() = 0;

    virtual bool     readBool  (const char label[]) = 0;
    virtual float    readFloat (const char label[]) = 0;
    virtual SkString readString(const char label[]) = 0;

    class AutoBlock final {
    public:
        explicit AutoBlock(const std::unique_ptr<StreamReader>& reader)
            : fReader(reader.get())
            , fType(reader->openBlock()) {}

        ~AutoBlock() {
            if (fType != BlockType::kEoB) {
                fReader->closeBlock();
            }
        }

        BlockType type() const { return fType; }

    private:
        StreamReader* fReader;
        BlockType     fType;
    };
};

}

#endif // SkRiveStreamreader_DEFINED
