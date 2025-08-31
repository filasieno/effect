#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

#include <print>
#include <liburing.h>

namespace ak {
    
    //  implementation 
    // ----------------------------------------------------------------------------------------------------------------

    // TaskContext ctor/dtor definitions
    CThread::Context::~Context() {
        using namespace priv;
        AK_ASSERT(state == CThread::State::DELETING);
        AK_ASSERT(is_AkDLink_detached(&tasklist_link));
        AK_ASSERT(is_AkDLink_detached(&wait_link));
        dump_task_count();
        check_invariants();
    }

    AkVoid* CThread::Context::operator new(std::size_t n) noexcept {
        AkVoid* mem = try_alloc_mem(n);
        if (!mem) return nullptr;
        return mem;
    }

    AkVoid CThread::Context::operator delete(AkVoid* ptr, std::size_t sz) {
        (AkVoid)sz;
        free_mem(ptr);
    }

    AkVoid CThread::Context::unhandled_exception() noexcept 
    {
        std::abort(); /* unreachable */
    }

    AkVoid CThread::Context::return_value(int value) noexcept {
        using namespace priv;

        check_invariants();

        CThread::Context* current_context = get_context(global_kernel_state.current_cthread);
        current_context->res = value;
        if (global_kernel_state.current_cthread == global_kernel_state.main_cthread) {
            std::print("MainTask done; returning: {}\n", value);
            global_kernel_state.main_cthread_exit_code = value;
        }

        // Wake up all tasks waiting for this task
        if (is_AkDLink_detached(&awaiter_list)) {
            return;
        }

        do {
            priv::AkDLink* next = dequeue_AkDLink(&awaiter_list);
            CThread::Context* ctx = get_linked_cthread_context(next);
            dump_task_count();
            AK_ASSERT(ctx->state == CThread::State::WAITING);
            --global_kernel_state.waiting_cthread_count;
            ctx->state = CThread::State::READY;
            enqueue_AkDLink(&global_kernel_state.ready_list, &ctx->wait_link);
            ++global_kernel_state.ready_cthread_count;
            dump_task_count();

        } while (!is_AkDLink_detached(&awaiter_list));

    }

    AkVoid CThread::Context::InitialSuspend::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        CThread::Context* promise = &hdl.promise();

        // Check initial preconditions
        AK_ASSERT(promise->state == CThread::State::CREATED);
        AK_ASSERT(is_AkDLink_detached(&promise->wait_link));
        check_invariants();

        // Add task to the kernel
        ++global_kernel_state.cthread_count;
        enqueue_AkDLink(&global_kernel_state.cthread_list, &promise->tasklist_link);

        ++global_kernel_state.ready_cthread_count;
        enqueue_AkDLink(&global_kernel_state.ready_list, &promise->wait_link);
        promise->state = CThread::State::READY;

        // Check post-conditions
        AK_ASSERT(promise->state == CThread::State::READY);
        AK_ASSERT(!is_AkDLink_detached(&promise->wait_link));
        check_invariants();
        dump_task_count();
    }

    CThread::Hdl CThread::Context::FinalSuspend::await_suspend(CThread::Hdl hdl) const noexcept {
        using namespace priv;

        // Check preconditions
        CThread::Context* ctx = &hdl.promise();
        AK_ASSERT(global_kernel_state.current_cthread == hdl);
        AK_ASSERT(ctx->state == CThread::State::RUNNING);
        AK_ASSERT(is_AkDLink_detached(&ctx->wait_link));
        check_invariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = CThread::State::ZOMBIE;
        ++global_kernel_state.zombie_cthread_count;
        enqueue_AkDLink(&global_kernel_state.zombie_list, &ctx->wait_link);
        global_kernel_state.current_cthread = CThread();
        check_invariants();

        return schedule_next_thread();
    }
}


