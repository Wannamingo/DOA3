/* Portable low-64-bit MMX semantics for the statically recompiled DOA3 core. */
#ifndef RECOMP_PORTABLE_MMX_H
#define RECOMP_PORTABLE_MMX_H
static inline RecompMmx MMX_PADDW(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=(uint16_t)(a.uw[i]+b.uw[i]); return r; }
static inline RecompMmx MMX_PSUBW(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=(uint16_t)(a.uw[i]-b.uw[i]); return r; }
static inline RecompMmx MMX_PMULLW(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=(uint16_t)((uint32_t)a.uw[i]*b.uw[i]); return r; }
static inline RecompMmx MMX_PMULHW(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=(uint16_t)((uint32_t)((int32_t)(int16_t)a.uw[i]*(int16_t)b.uw[i])>>16); return r; }
static inline RecompMmx MMX_PAVGB(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<8;++i) r.ub[i]=(uint8_t)(((unsigned)a.ub[i]+b.ub[i]+1)>>1); return r; }
static inline RecompMmx MMX_PUNPCKLWD(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<2;++i) {r.uw[2*i]=a.uw[i];r.uw[2*i+1]=b.uw[i];} return r; }
static inline RecompMmx MMX_PUNPCKHWD(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<2;++i) {r.uw[2*i]=a.uw[i+2];r.uw[2*i+1]=b.uw[i+2];} return r; }
static inline RecompMmx MMX_PSLLW(RecompMmx a, uint64_t count)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=count>15?0:(uint16_t)((uint32_t)a.uw[i]<<count); return r; }
static inline uint32_t recomp_signed_shift(uint32_t value, unsigned count, unsigned width)
{ const uint32_t mask=width==16?0xffffu:0xffffffffu; if(!count)return value&mask; const unsigned n=count>=width?width-1:count; const uint32_t fill=(value&(1u<<(width-1)))?mask^(mask>>n):0; return ((value>>n)|fill)&mask; }
static inline RecompMmx MMX_PSRAW(RecompMmx a, uint64_t count)
{ RecompMmx r; for (unsigned i=0;i<4;++i) r.uw[i]=(uint16_t)recomp_signed_shift(a.uw[i],count>15?15:(unsigned)count,16); return r; }
static inline RecompMmx MMX_PSRAD(RecompMmx a, uint64_t count)
{ RecompMmx r; for (unsigned i=0;i<2;++i) r.ud[i]=recomp_signed_shift(a.ud[i],count>31?31:(unsigned)count,32); return r; }
static inline RecompMmx MMX_PACKUSWB(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<8;++i) {int v=(int16_t)(i<4?a.uw[i]:b.uw[i-4]);r.ub[i]=(uint8_t)(v<0?0:v>255?255:v);} return r; }
static inline RecompMmx MMX_PACKSSDW(RecompMmx a, RecompMmx b)
{ RecompMmx r; for (unsigned i=0;i<4;++i) {int32_t v=(int32_t)(i<2?a.ud[i]:b.ud[i-2]);r.uw[i]=(uint16_t)(v < -32768 ? -32768 : v>32767?32767:v);} return r; }
#endif
