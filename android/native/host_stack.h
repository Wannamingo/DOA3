#pragma once
#include <exception>
#include <optional>
#include <type_traits>
extern "C" int recomp_host_needs_main_stack(void);
extern "C" void recomp_host_call_main(void (*entry)(void *),void *parameter);

// Synchronous host services must not yield or dispatch guest code. Exceptions
// return to the caller's own continuation rather than unwinding another stack.
template<class F> auto recomp_host_stack(F &&fn) -> std::invoke_result_t<F &> {
    using Result=std::invoke_result_t<F &>;
    std::exception_ptr error;
    if constexpr(std::is_void_v<Result>) {
        auto task=[&]{try{fn();}catch(...){error=std::current_exception();}};
        recomp_host_call_main([](void *p){(*static_cast<decltype(task) *>(p))();},&task);
        if(error)std::rethrow_exception(error);
    } else {
        std::optional<Result> result;
        auto task=[&]{try{result=fn();}catch(...){error=std::current_exception();}};
        recomp_host_call_main([](void *p){(*static_cast<decltype(task) *>(p))();},&task);
        if(error)std::rethrow_exception(error);
        return *result;
    }
}
