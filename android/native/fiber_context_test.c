#include "fiber_context.h"
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
static void *main_fiber;
static uint32_t passes;
static uintptr_t stack_low, stack_high;
static uint32_t host_calls;
static void on_host(void *opaque)
{
    int local;
    if(recomp_host_needs_main_stack() || (uintptr_t)&local<stack_low || (uintptr_t)&local>=stack_high
        || fegetround()!=FE_TONEAREST)abort();
    ++host_calls;
    *(uint32_t *)opaque=host_calls;
}
static void entry(void *opaque)
{
    int mode = (int)(intptr_t)opaque;
    volatile uint64_t local = UINT64_C(0x123456789abcdef0);
    if (fesetround(mode)) abort();
    for (;;) {
        if (local != UINT64_C(0x123456789abcdef0) || fegetround() != mode) abort();
        ++passes;
        uint32_t result=0;
        recomp_host_call_main(on_host,&result);
        if(!result || fegetround()!=mode)abort();
        SwitchToFiber(main_fiber);
    }
}
int main(void)
{
    pthread_attr_t attributes;void *stack;size_t bytes;
    if(pthread_getattr_np(pthread_self(),&attributes) || pthread_attr_getstack(&attributes,&stack,&bytes))return 4;
    pthread_attr_destroy(&attributes);stack_low=(uintptr_t)stack;stack_high=stack_low+bytes;
    main_fiber = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
    void *down = CreateFiberEx(0, 1048576, FIBER_FLAG_FLOAT_SWITCH, entry, (void *)(intptr_t)FE_DOWNWARD);
    void *up = CreateFiberEx(0, 1048576, FIBER_FLAG_FLOAT_SWITCH, entry, (void *)(intptr_t)FE_UPWARD);
    if (!main_fiber || !down || !up || fesetround(FE_TONEAREST)) return 1;
    for (unsigned i = 0; i < 10000; ++i) {
        SwitchToFiber(down); SwitchToFiber(up);
        if (fegetround() != FE_TONEAREST) return 2;
    }
    DeleteFiber(down); DeleteFiber(up);
    if (passes != 20000 || host_calls!=20000 || !ConvertFiberToThread()) return 3;
    puts("ARM64 fibers: 20000 switches and host callbacks on registered pthread stack with independent rounding modes passed.");
    return 0;
}
