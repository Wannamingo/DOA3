#include "d3d_vblank.h"
#include "xapi_time_adapter.h"
#include <chrono>
#include <cstdint>
#include <thread>
extern "C" void recomp_kernel_wait_for_vblank(uint64_t deadline_ns);
namespace {
using Clock=std::chrono::steady_clock;
Clock::time_point last,completed,presented;
constexpr auto interval=std::chrono::nanoseconds(1000000000/60);
Clock::time_point guest_now()
{return Clock::time_point(std::chrono::nanoseconds(recomp_xapi_performance_counter()));}
template<class Now> void sleep_until(Clock::time_point deadline,Now now)
{while(now()<deadline)std::this_thread::sleep_for(deadline-now());}
}
void recomp_d3d_vblank_reset(void) { last=completed=presented=guest_now(); }
void recomp_d3d_wait_vblank(void)
{
    const auto now=guest_now();
    if(last<=now)last+=interval*((now-last)/interval+1);
    const auto deadline=last;
    const auto ns=(uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(deadline.time_since_epoch()).count();
    recomp_kernel_wait_for_vblank(ns);
    if(!recomp_xapi_skip_wait(ns))sleep_until(deadline,guest_now);
    if(completed<deadline)completed=deadline;
}
void recomp_d3d_wait_present(void)
{if(completed<=presented)recomp_d3d_wait_vblank();presented=completed;}
void recomp_d3d_sleep_until(long long ns)
{sleep_until(Clock::time_point(std::chrono::nanoseconds(ns)),Clock::now);}
