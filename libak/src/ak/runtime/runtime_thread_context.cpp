#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <print>
#include <liburing.h>

// TaskContext ctor/dtor definitions
AkPromise::~AkPromise() {
    AK_ASSERT(state == AkCoroutineState::DELETING);
    AK_ASSERT(ak_is_dlink_detached(&tasklist_link));
    AK_ASSERT(ak_is_dlink_detached(&wait_link));
    ak::priv::dump_task_count();
    ak::priv::check_invariants();
}

AkVoid* AkPromise::operator new(std::size_t n) noexcept {
    AkVoid* mem = ak::try_alloc_mem(n);
    if (!mem) return nullptr;
    return mem;
}

AkVoid AkPromise::operator delete(AkVoid* ptr, std::size_t sz) {
    (AkVoid)sz;
    ak::free_mem(ptr);
}

AkVoid AkPromise::unhandled_exception() noexcept 
{
    std::abort(); /* unreachable */
}

AkVoid AkPromise::return_value(int value) noexcept {

    ak::priv::check_invariants();

    AkPromise* current_context = ak::get_context(global_kernel_state.current_cthread);
    current_context->res = value;
    if (global_kernel_state.current_cthread == global_kernel_state.main_cthread) {
        std::print("MainTask done; returning: {}\n", value);
        global_kernel_state.main_cthread_exit_code = value;
    }

    // Wake up all tasks waiting for this task
    if (ak_is_dlink_detached(&awaiter_list)) {
        return;
    }

    do {
        AkDLink* next = ak_dequeue_dlink(&awaiter_list);
        AkPromise* ctx = ak::priv::get_linked_cthread_context(next);
        ak::priv::dump_task_count();
        AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
        --global_kernel_state.waiting_cthread_count;
        ctx->state = AkCoroutineState::READY;
        ak_enqueue_dlink(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_cthread_count;
        ak::priv::dump_task_count();

    } while (!ak_is_dlink_detached(&awaiter_list));

}

AkVoid AkPromise::InitialSuspend::await_suspend(CThread::Hdl hdl) const noexcept {
    
    AkPromise* promise = &hdl.promise();

    // Check initial preconditions
    AK_ASSERT(promise->state == AkCoroutineState::CREATED);
    AK_ASSERT(ak_is_dlink_detached(&promise->wait_link));
    ak::priv::check_invariants();

    // Add task to the kernel
    ++global_kernel_state.cthread_count;
    ak_enqueue_dlink(&global_kernel_state.cthread_list, &promise->tasklist_link);

    ++global_kernel_state.ready_cthread_count;
    ak_enqueue_dlink(&global_kernel_state.ready_list, &promise->wait_link);
    promise->state = AkCoroutineState::READY;

    // Check post-conditions
    AK_ASSERT(promise->state == AkCoroutineState::READY);
    AK_ASSERT(!ak_is_dlink_detached(&promise->wait_link));
    ak::priv::check_invariants();
    ak::priv::dump_task_count();
}

CThread::Hdl AkPromise::FinalSuspend::await_suspend(CThread::Hdl hdl) const noexcept {
    // Check preconditions
    AkPromise* ctx = &hdl.promise();
    AK_ASSERT(global_kernel_state.current_cthread == hdl);
    AK_ASSERT(ctx->state == AkCoroutineState::RUNNING);
    AK_ASSERT(ak_is_dlink_detached(&ctx->wait_link));
    ak::priv::check_invariants();

    // Move the current task from RUNNING to ZOMBIE
    ctx->state = AkCoroutineState::ZOMBIE;
    ++global_kernel_state.zombie_cthread_count;
    ak_enqueue_dlink(&global_kernel_state.zombie_list, &ctx->wait_link);
    global_kernel_state.current_cthread = CThread();
    ak::priv::check_invariants();

    return ak::priv::schedule_next_thread();
}
