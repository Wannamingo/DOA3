// SPDX-License-Identifier: GPL-2.0-or-later
#include "host_diagnostics.h"
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <cstdio>
#include <initializer_list>
#include <sys/syscall.h>
#include <unwind.h>
#include <dlfcn.h>
namespace {
struct sigaction previous[NSIG];
_Unwind_Reason_Code frame(_Unwind_Context *context,void *opaque){
    unsigned &count=*static_cast<unsigned *>(opaque);
    if(count++>=48)return _URC_END_OF_STACK;
    uintptr_t pc=_Unwind_GetIP(context);Dl_info library{};
    if(pc&&dladdr(reinterpret_cast<void *>(pc),&library)){
        char line[384];int bytes=std::snprintf(line,sizeof line,"host-frame %s+0x%llx\n",
            library.dli_fname,(unsigned long long)(pc-(uintptr_t)library.dli_fbase));
        if(bytes>0)write(STDERR_FILENO,line,(size_t)bytes);
    }
    return _URC_NO_REASON;
}
void crash(int signal,siginfo_t *info,void *context)
{
    auto *state=(ucontext_t *)context;
    char line[256];int bytes=std::snprintf(line,sizeof line,
        "host-crash signal=%d address=%p pc=0x%llx sp=0x%llx tid=%ld\n",
        signal,info->si_addr,(unsigned long long)state->uc_mcontext.pc,
        (unsigned long long)state->uc_mcontext.sp,syscall(SYS_gettid));
    if(bytes>0)write(STDERR_FILENO,line,(size_t)bytes);
    unsigned count=0;_Unwind_Backtrace(frame,&count);
    sigaction(signal,&previous[signal],nullptr);
    syscall(SYS_tgkill,getpid(),syscall(SYS_gettid),signal);
}
}
extern "C" void recomp_install_host_diagnostics(void)
{
    struct sigaction action{};action.sa_sigaction=crash;sigemptyset(&action.sa_mask);action.sa_flags=SA_SIGINFO;
    for(int signal:{SIGSEGV,SIGBUS,SIGILL,SIGABRT,SIGFPE})sigaction(signal,&action,&previous[signal]);
}
