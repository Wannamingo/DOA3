#define RECOMP_FORCE_PORTABLE_SIMD
#include "recomp_types.h"
#include <fenv.h>
#include <stdio.h>

int main(void)
{
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const int positive[]={2,2,3,2},negative[]={-2,-3,-2,-2};
    for (unsigned i = 0; i < 4; ++i) {
        if (fesetround(modes[i]) || MMX_CVT_F2I(2.5f,0)!=positive[i] || MMX_CVT_F2I(-2.5f,0)!=negative[i] || MMX_CVT_F2I(-2.5f,1)!=-2) return 1;
    }
    if (fesetround(FE_TONEAREST)) return 2;
    if (MMX_CVT_F2I(INFINITY,0)!=INT32_MIN || MMX_CVT_F2I(NAN,0)!=INT32_MIN) return 3;
    if (MMX_CVT_F2I(2147483648.0f,0)!=INT32_MIN || MMX_CVT_F2I(-2147483648.0f,0)!=INT32_MIN) return 4;
    if (MMX_CVT_F2I(2147483520.0f,0)!=2147483520 || MMX_CVT_F2I(-2147483520.0f,0)!=-2147483520) return 5;
    RecompXmm a, b;
    a.u[0] = 0x7fc00013u; a.u[1] = 0x80000000u; a.u[2] = 0x7f800000u; a.u[3] = 0u;
    b.u[0] = 0xffc01024u; b.u[1] = 1u; b.u[2] = 0x12345678u; b.u[3] = 0xffffffffu;
    for (unsigned selector = 0; selector < 256; ++selector) {
        RecompXmm r = XMM_SHUFFLE(a, b, selector);
        if (r.u[0] != a.u[selector & 3] || r.u[1] != a.u[(selector >> 2) & 3] ||
            r.u[2] != b.u[(selector >> 4) & 3] || r.u[3] != b.u[selector >> 6]) return 6;
    }
    RecompMmx x={.uw={0xffff,0x8000,0x7fff,0x100}},y={.uw={1,2,3,4}};
    RecompMmx r=MMX_PADDW(x,y);if(r.uw[0] || r.uw[1]!=0x8002 || r.uw[2]!=0x8002 || r.uw[3]!=0x104)return 7;
    r=MMX_PSLLW(x,16);if(r.q)return 8;
    r=MMX_PSRAW(x,UINT64_MAX);if(r.uw[0]!=0xffff || r.uw[1]!=0xffff || r.uw[2] || r.uw[3])return 9;
    r=MMX_PACKUSWB(x,y);if(r.q!=UINT64_C(0x04030201ffff0000))return 10;
    r=MMX_PUNPCKHWD(x,y);if(r.uw[0]!=0x7fff || r.uw[1]!=3 || r.uw[2]!=0x100 || r.uw[3]!=4)return 11;
    puts("DOA3 ARM64 SIMD: rounding, invalid conversions, 256 SHUFPS selectors, MMX wrapping/saturation/shifts passed.");
    return 0;
}
