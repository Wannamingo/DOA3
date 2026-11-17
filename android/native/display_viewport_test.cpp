// SPDX-License-Identifier: GPL-2.0-or-later
#include "display_viewport.h"
#include <cstdio>

using namespace doa3_display;

int main()
{
    const struct { int w, h; Aspect aspect; Viewport expected; } cases[] = {
        {1920, 1080, Aspect::Source, {240, 0, 1440, 1080}},
        {1920, 1080, Aspect::Standard, {240, 0, 1440, 1080}},
        {1920, 1080, Aspect::Wide, {0, 0, 1920, 1080}},
        {1920, 1080, Aspect::Stretch, {0, 0, 1920, 1080}},
        {1600, 1200, Aspect::Wide, {0, 150, 1600, 900}},
        {1600, 1200, Aspect::Stretch, {0, 0, 1600, 1200}},
        {2560, 1600, Aspect::Stretch, {0, 0, 2560, 1600}},
        {0, 1200, Aspect::Stretch, {0, 0, 0, 0}},
        {1, 1, Aspect::Wide, {0, 0, 1, 1}},
    };
    for (const auto &c : cases) {
        const auto actual = viewport(c.w, c.h, c.aspect);
        if (actual.x != c.expected.x || actual.y != c.expected.y ||
            actual.width != c.expected.width || actual.height != c.expected.height) return 1;
    }
    if (parse("stretch") != Aspect::Stretch || parse(nullptr) != Aspect::Source ||
        parse("invalid") != Aspect::Source) return 2;
    std::puts("DOA3 viewport: fitted original/4:3/16:9 and full-surface stretch passed.");
    return 0;
}
