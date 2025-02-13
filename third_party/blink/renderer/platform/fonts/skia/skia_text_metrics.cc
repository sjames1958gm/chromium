// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/skia/skia_text_metrics.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

#include <SkFont.h>
#include <SkPath.h>

namespace blink {

namespace {

template <class T>
T* advance_by_byte_size(T* p, unsigned byte_size) {
  return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(p) + byte_size);
}

}  // namespace

void GetGlyphWidthForHarfBuzz(const SkFont& font,
                              hb_codepoint_t codepoint,
                              hb_position_t* width) {
  DCHECK_LE(codepoint, 0xFFFFu);
  CHECK(width);

  SkScalar sk_width;
  uint16_t glyph = codepoint;

  font.getWidths(&glyph, 1, &sk_width, nullptr);
  if (!font.isSubpixel())
    sk_width = SkScalarRoundToInt(sk_width);
  *width = SkiaScalarToHarfBuzzPosition(sk_width);
}

void GetGlyphWidthForHarfBuzz(const SkFont& font,
                              unsigned count,
                              hb_codepoint_t* glyphs,
                              unsigned glyph_stride,
                              hb_position_t* advances,
                              unsigned advance_stride) {
  // Batch the call to getWidths because its function entry cost is not
  // cheap. getWidths accepts multiple glyphd ID, but not from a sparse
  // array that copy them to a regular array.
  Vector<Glyph, 256> glyph_array(count);
  for (unsigned i = 0; i < count;
       i++, glyphs = advance_by_byte_size(glyphs, glyph_stride)) {
    glyph_array[i] = *glyphs;
  }
  Vector<SkScalar, 256> sk_width_array(count);
  font.getWidths(glyph_array.data(), count, sk_width_array.data(), nullptr);

  if (!font.isSubpixel()) {
    for (unsigned i = 0; i < count; i++)
      sk_width_array[i] = SkScalarRoundToInt(sk_width_array[i]);
  }

  // Copy the results back to the sparse array.
  for (unsigned i = 0; i < count;
       i++, advances = advance_by_byte_size(advances, advance_stride)) {
    *advances = SkiaScalarToHarfBuzzPosition(sk_width_array[i]);
  }
}

void GetGlyphExtentsForHarfBuzz(const SkFont& font,
                                hb_codepoint_t codepoint,
                                hb_glyph_extents_t* extents) {
  DCHECK_LE(codepoint, 0xFFFFu);
  CHECK(extents);

  SkRect sk_bounds;
  uint16_t glyph = codepoint;

  font.getWidths(&glyph, 1, nullptr, &sk_bounds);
  if (!font.isSubpixel()) {
    // Use roundOut() rather than round() to avoid rendering glyphs
    // outside the visual overflow rect. crbug.com/452914.
    sk_bounds.set(sk_bounds.roundOut());
  }

  // Invert y-axis because Skia is y-grows-down but we set up HarfBuzz to be
  // y-grows-up.
  extents->x_bearing = SkiaScalarToHarfBuzzPosition(sk_bounds.fLeft);
  extents->y_bearing = SkiaScalarToHarfBuzzPosition(-sk_bounds.fTop);
  extents->width = SkiaScalarToHarfBuzzPosition(sk_bounds.width());
  extents->height = SkiaScalarToHarfBuzzPosition(-sk_bounds.height());
}

void GetBoundsForGlyph(const SkFont& font, Glyph glyph, SkRect* bounds) {
#if defined(OS_MACOSX)
  // TODO(drott): Remove this once we have better metrics bounds
  // on Mac, https://bugs.chromium.org/p/skia/issues/detail?id=5328
  SkPath path;
  font.getPath(glyph, &path);
  *bounds = path.getBounds();
#else
  font.getWidths(&glyph, 1, nullptr, bounds);
#endif

  if (!font.isSubpixel()) {
    SkIRect ir;
    bounds->roundOut(&ir);
    bounds->set(ir);
  }
}

void GetBoundsForGlyphs(const SkFont& font,
                        const Vector<Glyph, 256>& glyphs,
                        SkRect* bounds) {
#if defined(OS_MACOSX)
  for (unsigned i = 0; i < glyphs.size(); i++) {
    GetBoundsForGlyph(font, glyphs[i], &bounds[i]);
  }
#else
  static_assert(sizeof(Glyph) == 2, "Skia expects 2 bytes glyph id.");
  font.getWidths(glyphs.data(), glyphs.size(), nullptr, bounds);

  if (!font.isSubpixel()) {
    for (unsigned i = 0; i < glyphs.size(); i++) {
      SkIRect ir;
      bounds[i].roundOut(&ir);
      bounds[i].set(ir);
    }
  }
#endif
}

float GetWidthForGlyph(const SkFont& font, Glyph glyph) {
  SkScalar sk_width;
  font.getWidths(&glyph, 1, &sk_width, nullptr);

  if (!font.isSubpixel())
    sk_width = SkScalarRoundToInt(sk_width);

  return SkScalarToFloat(sk_width);
}

hb_position_t SkiaScalarToHarfBuzzPosition(SkScalar value) {
  // We treat HarfBuzz hb_position_t as 16.16 fixed-point.
  static const int kHbPosition1 = 1 << 16;
  return clampTo<int>(value * kHbPosition1);
}

}  // namespace blink
