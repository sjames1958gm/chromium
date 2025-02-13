// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SKIA_SKIA_TEXT_METRICS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_SKIA_SKIA_TEXT_METRICS_H_

#include "third_party/blink/renderer/platform/fonts/glyph.h"

#include <hb.h>
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/skia/include/core/SkRect.h"

class SkFont;

namespace blink {

void GetGlyphWidthForHarfBuzz(const SkFont&,
                              hb_codepoint_t,
                              hb_position_t* width);
void GetGlyphWidthForHarfBuzz(const SkFont&,
                              unsigned count,
                              hb_codepoint_t* first_glyph,
                              unsigned glyph_stride,
                              hb_position_t* first_advance,
                              unsigned advance_stride);
void GetGlyphExtentsForHarfBuzz(const SkFont&,
                                hb_codepoint_t,
                                hb_glyph_extents_t*);

void GetBoundsForGlyph(const SkFont&, Glyph, SkRect* bounds);
void GetBoundsForGlyphs(const SkFont&, const Vector<Glyph, 256>&, SkRect*);
float GetWidthForGlyph(const SkFont&, Glyph);

hb_position_t SkiaScalarToHarfBuzzPosition(SkScalar value);

}  // namespace blink

#endif
