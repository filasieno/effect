#pragma once

#include "core.hpp"

namespace ak {

    // TaskContext implementation 
    // ----------------------------------------------------------------------------------------------------------------

    inline void TaskContext::InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        TaskContext* promise = &hdl.promise();

        // Check initial preconditions
        assert(promise->state == TaskState::CREATED);
        assert(IsLinkDetached(&promise->waitLink));
        CheckInvariants();

        // Add task to the kernel
        ++gKernel.taskCount;
        EnqueueLink(&gKernel.taskList, &promise->taskListLink);

        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &promise->waitLink);
        promise->state = TaskState::READY;

        // Check post-conditions
        assert(promise->state == TaskState::READY);
        assert(!IsLinkDetached(&promise->waitLink));
        CheckInvariants();
        priv::DebugTaskCount();
    }

    inline TaskHdl TaskContext::FinalSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace priv;
        using namespace utl;

        // Check preconditions
        TaskContext* ctx = &hdl.promise();
        assert(gKernel.currentTaskHdl == hdl);
        assert(ctx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&ctx->waitLink));
        CheckInvariants();

        // Move the current task from RUNNING to ZOMBIE
        ctx->state = TaskState::ZOMBIE;
        ++gKernel.zombieCount;
        EnqueueLink(&gKernel.zombieList, &ctx->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }

    // TaskContext ctor/dtor definitions
    inline TaskContext::~TaskContext() {
        using namespace priv;
        assert(state == TaskState::DELETING);
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        DebugTaskCount();
        CheckInvariants();
    }

    template <typename... Args>
    inline TaskContext::TaskContext(Args&&... ) {
        using namespace priv;

        InitLink(&taskListLink);
        InitLink(&waitLink);
        InitLink(&awaitingTerminationList);
        state = TaskState::CREATED;
        enqueuedIO = 0;
        ioResult = -1;

        // Check post-conditions
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    inline void* TaskContext::operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    inline void TaskContext::operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }


    inline void TaskContext::return_void() noexcept {
        using namespace priv;

        CheckInvariants();

        // Wake up all tasks waiting for this task
        if (IsLinkDetached(&awaitingTerminationList)) {
            return;
        }

        do {
            Link* next = DequeueLink(&awaitingTerminationList);
            TaskContext* ctx = GetLinkedTaskContext(next);
            DebugTaskCount();
            assert(ctx->state == TaskState::WAITING);
            --gKernel.waitingCount;
            ctx->state = TaskState::READY;
            EnqueueLink(&gKernel.readyList, &ctx->waitLink);
            ++gKernel.readyCount;
            DebugTaskCount();

        } while (!IsLinkDetached(&awaitingTerminationList));
    }


    // SuspendOp implmentation
    // ----------------------------------------------------------------------------------------------------------------

    inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(gKernel.currentTaskHdl);

        TaskContext* currentPromise = &currentTask.promise();

        if constexpr (IS_DEBUG_MODE) {
            assert(gKernel.currentTaskHdl == currentTask);
            assert(currentPromise->state == TaskState::RUNNING);
            assert(IsLinkDetached(&currentPromise->waitLink));
            CheckInvariants();
        }

        // Move the current task from RUNNINIG to READY
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        return ScheduleNextTask();
    }
    
    // ResumeTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
        using namespace priv;
        using namespace utl;

        assert(gKernel.currentTaskHdl == currentTaskHdl);

        // Check the current Task
        TaskContext* currentPromise = &gKernel.currentTaskHdl.promise();
        assert(IsLinkDetached(&currentPromise->waitLink));
        assert(currentPromise->state == TaskState::RUNNING);
        CheckInvariants();

        // Suspend the current Task
        currentPromise->state = TaskState::READY;
        ++gKernel.readyCount;
        EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();

        // Move the target task from READY to RUNNING
        TaskContext* promise = &hdl.promise();
        promise->state = TaskState::RUNNING;
        DetachLink(&promise->waitLink);
        --gKernel.readyCount;
        gKernel.currentTaskHdl = hdl;
        CheckInvariants();

        assert(gKernel.currentTaskHdl);
        return hdl;
    }

    // JoinTaskOp implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept
    {
        using namespace priv;
        using namespace utl;

        TaskContext* currentTaskCtx = &currentTaskHdl.promise();

        // Check CurrentTask preconditions
        assert(currentTaskCtx->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentTaskCtx->waitLink));
        assert(gKernel.currentTaskHdl == currentTaskHdl);
        CheckInvariants();

        TaskContext* joinedTaskCtx = &joinedTaskHdl.promise();                
        TaskState joinedTaskState = joinedTaskCtx->state;
        switch (joinedTaskState) {
            case TaskState::READY:
            {

                // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the joined TASK from READY to RUNNING
                joinedTaskCtx->state = TaskState::RUNNING;
                DetachLink(&joinedTaskCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = joinedTaskHdl;
                CheckInvariants();
                DebugTaskCount();
                return joinedTaskHdl;
            }

            case TaskState::IO_WAITING:
            case TaskState::WAITING:
            {
                 // Move current Task from READY to WAITING
                currentTaskCtx->state = TaskState::WAITING;
                ++gKernel.waitingCount;
                EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
                ClearTaskHdl(&gKernel.currentTaskHdl);
                CheckInvariants();
                DebugTaskCount();

                // Move the Scheduler Task from READY to RUNNING
                TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
                assert(schedCtx->state == TaskState::READY);
                schedCtx->state = TaskState::RUNNING;
                DetachLink(&schedCtx->waitLink);
                --gKernel.readyCount;
                gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
                CheckInvariants();
                DebugTaskCount();

                return gKernel.schedulerTaskHdl;
            }
            
            case TaskState::DELETING:
            case TaskState::ZOMBIE:
            {
                return currentTaskHdl;
            }
            
            case TaskState::INVALID:
            case TaskState::CREATED:
            case TaskState::RUNNING:
            default:
            {
                // Illegal State
                abort();
            }
        }
    }

    // Task API Implementation
    // ----------------------------------------------------------------------------------------------------------------

    inline void ClearTaskHdl(TaskHdl* hdl) noexcept { *hdl = TaskHdl{}; }

    inline bool IsTaskHdlValid(TaskHdl hdl) noexcept { return hdl.address() != nullptr; }

    inline TaskContext* GetTaskContext(TaskHdl hdl) noexcept { return &hdl.promise(); }

    inline TaskContext* GetTaskContext() noexcept { return &gKernel.currentTaskHdl.promise(); }

    inline constexpr GetCurrentTaskOp GetCurrentTask() noexcept { return {}; }

    inline constexpr SuspendOp SuspendTask() noexcept { return {}; }

    inline JoinTaskOp JoinTask(TaskHdl hdl) noexcept { return JoinTaskOp(hdl); }

    inline JoinTaskOp operator co_await(TaskHdl hdl) noexcept { return JoinTaskOp(hdl); }

    inline TaskState GetTaskState(TaskHdl hdl) noexcept { return hdl.promise().state; }

    inline bool IsTaskDone(TaskHdl hdl) noexcept { return hdl.done(); }

    inline ResumeTaskOp ResumeTask(TaskHdl hdl) noexcept { return ResumeTaskOp(hdl); }

}