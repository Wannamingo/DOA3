#ifndef DOA3_ANDROID_FIBER_CONTEXT_H
#define DOA3_ANDROID_FIBER_CONTEXT_H
#include <stddef.h>
typedef void *LPVOID;
typedef void (*LPFIBER_START_ROUTINE)(void *);
int IsThreadAFiber(void);
void *GetCurrentFiber(void);
#define VOID void
#define WINAPI
#define FIBER_FLAG_FLOAT_SWITCH 1
void *ConvertThreadToFiberEx(void *parameter, unsigned flags);
int ConvertFiberToThread(void);
void *CreateFiberEx(size_t committed, size_t reserved, unsigned flags,
                    void (*entry)(void *), void *parameter);
void SwitchToFiber(void *fiber);
void DeleteFiber(void *fiber);
int recomp_host_needs_main_stack(void);
void recomp_host_call_main(void (*entry)(void *), void *parameter);
#endif
