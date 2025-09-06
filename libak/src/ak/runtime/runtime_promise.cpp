#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

// #include <print>
#include <liburing.h>

// TaskContext ctor/dtor definitions
AkPromise::~AkPromise()
{
    AK_ASSERT(state == AkCoroutineState::DELETING);
    AK_ASSERT(ak_dlink_is_detached(&tasklist_link));
    AK_ASSERT(ak_dlink_is_detached(&wait_link));
    runtime_dump_task_count();
    runtime_check_invariants();
}

void* AkPromise::operator new(std::size_t n) noexcept
{
    void* mem = ak_alloc_mem(n);
    if (!mem) return nullptr;
    return mem;
}

void AkPromise::operator delete(void* ptr, std::size_t sz)
{
    (void)sz;
    ak_free_mem(ptr);
}

void AkPromise::unhandled_exception() noexcept 
{
    std::abort(); /* unreachable */
}

void AkPromise::return_value(int value) noexcept
{

    runtime_check_invariants();

    AkPromise* current_context = ak_get_promise(global_kernel_state.current_task);
    current_context->res = value;
    if (global_kernel_state.current_task == global_kernel_state.main_task) {
        global_kernel_state.main_task_exit_code = value;
    }

    // Wake up all tasks waiting for this task
    if (ak_dlink_is_detached(&awaiter_list)) {
        return;
    }

    do {
        ak_dlink* next = ak_dlink_dequeue(&awaiter_list);
        AkPromise* ctx = runtime_get_linked_task_context(next);
        runtime_dump_task_count();
        AK_ASSERT(ctx->state == AkCoroutineState::WAITING);
        --global_kernel_state.waiting_task_count;
        ctx->state = AkCoroutineState::READY;
        ak_dlink_enqueue(&global_kernel_state.ready_list, &ctx->wait_link);
        ++global_kernel_state.ready_task_count;
        runtime_dump_task_count();

    } while (!ak_dlink_is_detached(&awaiter_list));

}

void AkPromise::InitialSuspend::await_suspend(AkCoroutineHandle hdl) const noexcept
{
    
    AkPromise* promise = &hdl.promise();

    // Check initial preconditions
    AK_ASSERT(promise->state == AkCoroutineState::CREATED);
    AK_ASSERT(ak_dlink_is_detached(&promise->wait_link));
    runtime_check_invariants();

    // Add task to the kernel
    ++global_kernel_state.task_count;
    ak_dlink_enqueue(&global_kernel_state.task_list, &promise->tasklist_link);

    ++global_kernel_state.ready_task_count;
    ak_dlink_enqueue(&global_kernel_state.ready_list, &promise->wait_link);
    promise->state = AkCoroutineState::READY;

    // Check post-conditions
    AK_ASSERT(promise->state == AkCoroutineState::READY);
    AK_ASSERT(!ak_dlink_is_detached(&promise->wait_link));
    runtime_check_invariants();
    runtime_dump_task_count();
}

AkCoroutineHandle AkPromise::FinalSuspend::await_suspend(AkCoroutineHandle hdl) const noexcept
{
    // Check preconditions
    AkPromise* ctx = &hdl.promise();
    AK_ASSERT(global_kernel_state.current_task == hdl);
    AK_ASSERT(ctx->state == AkCoroutineState::RUNNING);
    AK_ASSERT(ak_dlink_is_detached(&ctx->wait_link));
    runtime_check_invariants();

    // Move the current task from RUNNING to ZOMBIE
    ctx->state = AkCoroutineState::ZOMBIE;
    ++global_kernel_state.zombie_task_count;
    ak_dlink_enqueue(&global_kernel_state.zombie_list, &ctx->wait_link);
    global_kernel_state.current_task = AkTask();
    runtime_check_invariants();

    return runtime_schedule_next_thread();
}
