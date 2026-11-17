// SPDX-License-Identifier: GPL-2.0-or-later
#include "d3d_vblank.h"
#include "xapi_time_adapter.h"
#include <cstdio>

namespace {
constexpr uint64_t frame = 1000000000u / 60u;
uint64_t now = 42;
unsigned waits = 0;
}

uint64_t recomp_xapi_performance_counter(void) { return now; }
bool recomp_xapi_skip_wait(uint64_t deadline) { now = deadline; return true; }
extern "C" void recomp_kernel_wait_for_vblank(uint64_t) { ++waits; }

int main()
{
    recomp_d3d_vblank_reset();
    const uint64_t start = now;
    for (unsigned n = 0; n < 60; ++n) {
        // Alternate implicit presentation waits and waits issued by the game.
        if (n % 2) recomp_d3d_wait_vblank();
        recomp_d3d_wait_present();
        if (now != start + (n + 1) * frame || waits != n + 1) return 1;
    }
    // A frame that overruns by a little keeps the original Xbox tick phase.
    now += frame + 100;
    recomp_d3d_wait_present();
    if (now != start + 62 * frame || waits != 61) return 2;
    recomp_d3d_wait_present();
    if (now != start + 63 * frame || waits != 62) return 3;
    std::puts("Guest pacing: 60 ticks per second, explicit wait reused, late frame recovers.");
    return 0;
}
