// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <cstring>

namespace doa3_display {
enum class Aspect : unsigned { Source, Standard, Wide, Stretch };
struct Viewport { int x, y, width, height; };

inline Aspect parse(const char *value)
{
    if (value && std::strcmp(value, "stretch") == 0) return Aspect::Stretch;
    if (value && std::strcmp(value, "16:9") == 0) return Aspect::Wide;
    if (value && std::strcmp(value, "4:3") == 0) return Aspect::Standard;
    return Aspect::Source;
}

inline const char *label(Aspect aspect)
{
    switch (aspect) {
    case Aspect::Stretch: return "stretch";
    case Aspect::Wide: return "16:9";
    case Aspect::Standard: return "4:3";
    default: return "source (4:3)";
    }
}

inline Viewport viewport(int width, int height, Aspect aspect)
{
    if (width <= 0 || height <= 0) return {};
    if (aspect == Aspect::Stretch) return {0, 0, width, height};

    const int numerator = aspect == Aspect::Wide ? 16 : 4;
    const int denominator = aspect == Aspect::Wide ? 9 : 3;
    const int64_t horizontal = int64_t(height) * numerator;
    const int64_t vertical = int64_t(width) * denominator;
    int w = width, h = height;
    if (horizontal <= vertical) {
        w = int(horizontal / denominator);
    } else {
        h = int(vertical / numerator);
    }
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    return {(width - w) / 2, (height - h) / 2, w, h};
}
}
