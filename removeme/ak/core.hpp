#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <cstdint>
#include <cstring>
#include <utility>
#include <immintrin.h>
#include "liburing.h"

#include "ak/types.hpp"
#include "ak/dlist.hpp"

namespace ak {

struct DefineTask;
struct TaskContext;

/// \brief Coroutine handle for a Task
/// \ingroup Task
using TaskHdl = std::coroutine_handle<TaskContext>;

/// \brief Defines a Task function type-erased pointer (no std::function)
/// \ingroup Task
template <typename... Args>
using TaskFn = DefineTask(*)(Args...);

namespace internal {

#ifdef NDEBUG
    constexpr bool IS_DEBUG_MODE    = false;
#else
    constexpr bool IS_DEBUG_MODE    = true;   
#endif
    constexpr bool TRACE_DEBUG_CODE = false;

    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

    // -------------------- Allocator types --------------------
    enum class AllocState {
        INVALID              = 0b0000,
        USED                 = 0b0010,
        FREE                 = 0b0001,
        WILD_BLOCK           = 0b0011,
        BEGIN_SENTINEL       = 0b0100,
        LARGE_BLOCK_SENTINEL = 0b0110,
        END_SENTINEL         = 0b1100,
    };
    
    struct AllocSizeRecord {
        U64 size      : 48;
        U64 state     : 4;
        U64 _reserved : 12;
    };

    constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
    constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
    constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

    struct AllocHeader {
        AllocSizeRecord thisSize;
        AllocSizeRecord prevSize;
    };

    struct FreeAllocHeader {
        AllocSizeRecord thisSize;
        AllocSizeRecord prevSize;
        DLink freeListLink;
    };
    static_assert(sizeof(FreeAllocHeader) == 32);

    static constexpr int ALLOCATOR_BIN_COUNT = 256;
    
    struct AllocStats {
        Size binAllocCount[ALLOCATOR_BIN_COUNT];
        Size binReallocCount[ALLOCATOR_BIN_COUNT];
        Size binFreeCount[ALLOCATOR_BIN_COUNT];
        Size binSplitCount[ALLOCATOR_BIN_COUNT];
        Size binMergeCount[ALLOCATOR_BIN_COUNT];
        Size binReuseCount[ALLOCATOR_BIN_COUNT];
        Size binPoolCount[ALLOCATOR_BIN_COUNT];
    };

    struct AllocTable {
        alignas(64) __m256i freeListbinMask;                         
        alignas(64) DLink    freeListBins[ALLOCATOR_BIN_COUNT];
        alignas(64) U32      freeListBinsCount[ALLOCATOR_BIN_COUNT];

        alignas(8) char*       heapBegin;
        alignas(8) const char* heapEnd;
        alignas(8) char*       memBegin;
        alignas(8) char*       memEnd;

        Size memSize;
        Size usedMemSize;
        Size freeMemSize;
        Size maxFreeBlockSize;

        AllocStats stats;

        alignas(8) FreeAllocHeader* beginSentinel;
        alignas(8) FreeAllocHeader* wildBlock;
        alignas(8) FreeAllocHeader* largeBlockSentinel;
        alignas(8) FreeAllocHeader* endSentinel;
    };

    struct Kernel {
        alignas(64) AllocTable allocTable;
        TaskHdl currentTaskHdl;
        TaskHdl schedulerTaskHdl;
        DLink   readyList;
        DLink   taskList;
        void*   mem;
        Size    memSize;
        alignas(64) io_uring ioRing;
        unsigned ioEntryCount;
        alignas(64) KernelTaskHdl kernelTask;
        DLink         zombieList;
        alignas(64) int taskCount;
        int readyCount;
        int waitingCount;
        int ioWaitingCount;
        int zombieCount;
        int interrupted;
    };
    
    extern struct Kernel gKernel;

    struct InitialSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        void           await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {}
    };

    struct FinalSuspendTaskOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept { assert(false); }
    };

    struct ResumeTaskOp {
        explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};
        constexpr bool await_ready() const noexcept { return hdl.done();}
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}
        TaskHdl hdl;
    };

    struct JoinTaskOp {
        explicit JoinTaskOp(TaskHdl hdl) : joinedTaskHdl(hdl) {};
        constexpr bool await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}
        TaskHdl joinedTaskHdl;
    };

    struct SuspendOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {};
    };

    struct GetCurrentTaskOp {
        constexpr bool    await_ready() const noexcept        { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept { this->hdl = hdl; return hdl; }
        constexpr TaskHdl await_resume() const noexcept       { return hdl; }
        TaskHdl hdl;
    };

    void CheckInvariants() noexcept;
    void DebugTaskCount() noexcept;

} // namespace internal

struct TaskContext {
    using Link = internal::DLink;

    void* operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    void  operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    template <typename... Args>
    TaskContext(Args&&... ) {
        using namespace internal;
        InitLink(&taskListLink);
        InitLink(&waitLink);
        InitLink(&awaitingTerminationList);
        state = TaskState::CREATED;
        enqueuedIO = 0;
        ioResult = -1;
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        assert(state == TaskState::CREATED);
        CheckInvariants();
    }

    ~TaskContext() {
        using namespace internal;
        assert(state == TaskState::DELETING);
        assert(IsLinkDetached(&taskListLink));
        assert(IsLinkDetached(&waitLink));
        DebugTaskCount();
        CheckInvariants();
    }

    TaskHdl        get_return_object() noexcept    { return TaskHdl::from_promise(*this);} 
    constexpr auto initial_suspend() noexcept      { return internal::InitialSuspendTaskOp{}; }
    constexpr auto final_suspend() noexcept        { return internal::FinalSuspendTaskOp{}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept  { assert(false); }

    TaskState state;
    int       ioResult;
    unsigned  enqueuedIO;
    Link      waitLink;
    Link      taskListLink;
    Link      awaitingTerminationList;
};

struct DefineTask {
    using promise_type = TaskContext;
    DefineTask(const TaskHdl& hdl) : hdl(hdl) {}
    operator TaskHdl() const noexcept { return hdl; }
    TaskHdl hdl;
};

inline void ClearTaskHdl(TaskHdl* hdl) noexcept { *hdl = TaskHdl{}; }
inline bool IsTaskHdlValid(TaskHdl hdl) { return hdl.address() != nullptr; }
inline TaskContext* GetTaskContext(TaskHdl hdl) { return &hdl.promise(); }
inline TaskContext* GetTaskContext() { return &internal::gKernel.currentTaskHdl.promise(); }
inline constexpr auto GetCurrentTask() noexcept { return internal::GetCurrentTaskOp{}; }
inline constexpr auto SuspendTask() noexcept { return internal::SuspendOp{}; }
inline auto JoinTask(TaskHdl hdl) noexcept { return internal::JoinTaskOp{hdl}; }
inline auto operator co_await(TaskHdl hdl) noexcept { return internal::JoinTaskOp{hdl}; }
inline TaskState GetTaskState(TaskHdl hdl) noexcept { return hdl.promise().state; }
inline bool IsTaskDone(TaskHdl hdl) noexcept { return hdl.done(); }
inline auto ResumeTask(TaskHdl hdl) noexcept { return internal::ResumeTaskOp{hdl}; }

struct KernelConfig { void* mem; Size memSize; unsigned ioEntryCount; };

namespace internal {

inline static TaskContext* GetLinkedTaskContext(const DLink* link) noexcept {
    unsigned long long promise_off = ((unsigned long long)link) - offsetof(TaskContext, waitLink);
    return (TaskContext*)promise_off;
}
struct RunSchedulerTaskOp { constexpr bool await_ready() const noexcept  { return false; } constexpr void await_resume() const noexcept {}  TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept; };
struct TerminateSchedulerOp { constexpr bool await_ready() const noexcept { return false; } KernelTaskHdl await_suspend(TaskHdl hdl) const noexcept; constexpr void await_resume() const noexcept {} }; 

inline auto RunSchedulerTask() noexcept {  
    return RunSchedulerTaskOp{}; 
}

inline auto TerminateSchedulerTask() noexcept { 
        return TerminateSchedulerOp{}; 
}

inline void DestroySchedulerTask(TaskHdl hdl) noexcept;

struct KernelTaskPromise {
    template <typename... Args>
    KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept {}
    void* operator new(std::size_t n) noexcept { void* mem = std::malloc(n); if (!mem) return nullptr; return mem; }
    void  operator delete(void* ptr, std::size_t sz) { (void)sz; std::free(ptr); }
    KernelTaskHdl  get_return_object() noexcept   { return KernelTaskHdl::from_promise(*this); }
    constexpr auto initial_suspend() noexcept     { return std::suspend_always {}; }
    constexpr auto final_suspend() noexcept       { return std::suspend_never  {}; }
    constexpr void return_void() noexcept         { }
    constexpr void unhandled_exception() noexcept { assert(false); }
};

struct DefineKernelTask { using promise_type = KernelTaskPromise; DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {} operator KernelTaskHdl() const noexcept { return hdl; } KernelTaskHdl hdl; };

inline TaskHdl ScheduleNextTask() noexcept;

int InitAllocTable(void* mem, Size size) noexcept;

inline int InitKernel(KernelConfig* config) noexcept {
    using namespace internal;
    if (InitAllocTable(config->mem, config->memSize) != 0) { return -1; }
    int res = io_uring_queue_init(config->ioEntryCount, &gKernel.ioRing, 0);
    if (res < 0) { std::print("io_uring_queue_init failed\n"); return -1; }
    gKernel.mem = config->mem; gKernel.memSize = config->memSize; gKernel.taskCount = 0; gKernel.readyCount = 0; gKernel.waitingCount = 0; gKernel.ioWaitingCount = 0; gKernel.zombieCount = 0; gKernel.interrupted = 0;
    ClearTaskHdl(&gKernel.currentTaskHdl); ClearTaskHdl(&gKernel.schedulerTaskHdl);
    InitLink(&gKernel.zombieList); InitLink(&gKernel.readyList); InitLink(&gKernel.taskList);
    return 0;
}

inline void FiniKernel() noexcept { io_uring_queue_exit(&gKernel.ioRing); }

// inline void InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept;
// inline TaskHdl FinalSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept;
// inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept;
// inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept;
// inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept;

inline void DebugTaskCount() noexcept;
inline void DoCheckTaskCountInvariant() noexcept;
inline void CheckTaskCountInvariant() noexcept;
inline void CheckInvariants() noexcept;

#ifdef AK_IMPLEMENTATION    
    alignas(64) struct Kernel gKernel;
#endif

} // namespace internal


//inline void TaskContext::return_void() noexcept;

} // namespace ak

// ======================= IMPLEMENTATIONS =======================
namespace ak { namespace internal {

inline TaskHdl RunSchedulerTaskOp::await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
    using namespace internal;
    (void)currentTaskHdl;
    TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();
    assert(gKernel.taskCount == 1);
    assert(gKernel.readyCount == 1);
    assert(schedulerPromise.state == TaskState::READY);
    assert(!IsLinkDetached(&schedulerPromise.waitLink));
    assert(gKernel.currentTaskHdl == TaskHdl());
    gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
    schedulerPromise.state = TaskState::RUNNING;
    DetachLink(&schedulerPromise.waitLink);
    --gKernel.readyCount;
    CheckInvariants();
    return gKernel.schedulerTaskHdl;
}

inline KernelTaskHdl TerminateSchedulerOp::await_suspend(TaskHdl hdl) const noexcept {
    using namespace internal;
    assert(gKernel.currentTaskHdl == gKernel.schedulerTaskHdl);
    assert(gKernel.currentTaskHdl == hdl);
    TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();
    assert(schedulerPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&schedulerPromise.waitLink));
    schedulerPromise.state = TaskState::ZOMBIE;
    ClearTaskHdl(&gKernel.currentTaskHdl);
    EnqueueLink(&gKernel.zombieList, &schedulerPromise.waitLink);
    ++gKernel.zombieCount;
    return gKernel.kernelTask;
}

inline void DestroySchedulerTask(TaskHdl hdl) noexcept {
    using namespace internal;
    TaskContext* promise = &hdl.promise();
    DetachLink(&promise->taskListLink);
    --gKernel.taskCount;
    DetachLink(&promise->waitLink);
    --gKernel.zombieCount;
    promise->state = TaskState::DELETING;
    hdl.destroy();
}

inline TaskHdl ScheduleNextTask() noexcept {
    using namespace internal;
    while (true) {
        if (gKernel.readyCount > 0) {
            DLink* link = DequeueLink(&gKernel.readyList);
            TaskContext* ctx = GetLinkedTaskContext(link);
            TaskHdl task = TaskHdl::from_promise(*ctx);
            assert(ctx->state == TaskState::READY);
            ctx->state = TaskState::RUNNING;
            --gKernel.readyCount;
            gKernel.currentTaskHdl = task;
            CheckInvariants();
            return task;
        }

        if (gKernel.ioWaitingCount > 0) {
            unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
            if (ready > 0) {
                int ret = io_uring_submit(&gKernel.ioRing);
                if (ret < 0) { std::print("io_uring_submit failed\n"); fflush(stdout); abort(); }
            }
            struct io_uring_cqe *cqe; unsigned head; unsigned completed = 0;
            io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                assert(ctx->state == TaskState::IO_WAITING);
                --gKernel.ioWaitingCount;
                ctx->state = TaskState::READY;
                ++gKernel.readyCount;
                EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                ctx->ioResult = cqe->res;
                --ctx->enqueuedIO;
                ++completed;
            }
            io_uring_cq_advance(&gKernel.ioRing, completed);
            continue;
        }

        while (gKernel.zombieCount > 0) {
            DebugTaskCount();
            DLink* zombieNode = DequeueLink(&gKernel.zombieList);
            TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
            assert(zombiePromise.state == TaskState::ZOMBIE);
            --gKernel.zombieCount;
            DetachLink(&zombiePromise.waitLink);
            DetachLink(&zombiePromise.taskListLink);
            --gKernel.taskCount;
            zombiePromise.state = TaskState::DELETING;
            TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
            zombieTaskHdl.destroy();
            DebugTaskCount();
        }

        if (gKernel.readyCount == 0) { abort(); }
    }
    abort();
}

template <typename... Args>
inline DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept {
    using namespace internal;
    TaskHdl mainTask = mainProc(args...);
    assert(!mainTask.done());
    assert(GetTaskState(mainTask) == TaskState::READY);
    while (true) {
        unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
        if (ready > 0) {
            int ret = io_uring_submit(&gKernel.ioRing);
            if (ret < 0) { std::print("io_uring_submit failed\n"); fflush(stdout); abort(); }
        }
        if (gKernel.readyCount > 0) {
            DLink* nextNode = gKernel.readyList.prev;
            TaskContext* nextPromise = GetLinkedTaskContext(nextNode);
            TaskHdl nextTask = TaskHdl::from_promise(*nextPromise);
            assert(nextTask != gKernel.schedulerTaskHdl);
            co_await ResumeTaskOp(nextTask);
            assert(gKernel.currentTaskHdl);
            continue;
        }
        while (gKernel.zombieCount > 0) {
            DebugTaskCount();
            DLink* zombieNode = DequeueLink(&gKernel.zombieList);
            TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
            assert(zombiePromise.state == TaskState::ZOMBIE);
            --gKernel.zombieCount;
            DetachLink(&zombiePromise.waitLink);
            DetachLink(&zombiePromise.taskListLink);
            --gKernel.taskCount;
            zombiePromise.state = TaskState::DELETING;
            TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
            zombieTaskHdl.destroy();
            DebugTaskCount();
        }
        bool waitingCC = gKernel.ioWaitingCount;
        if (waitingCC) {
            struct io_uring_cqe *cqe; unsigned head; unsigned completed = 0;
            io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                assert(ctx->state == TaskState::IO_WAITING);
                --gKernel.ioWaitingCount; ctx->state = TaskState::READY; ++gKernel.readyCount; EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                ctx->ioResult = cqe->res; --ctx->enqueuedIO; ++completed;
            }
            io_uring_cq_advance(&gKernel.ioRing, completed);
        }
        if (gKernel.readyCount == 0 && gKernel.ioWaitingCount == 0) { break; }
    }
    co_await TerminateSchedulerTask();
    assert(false); co_return;
}

template <typename... Args>
inline DefineKernelTask KernelTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args ... args) noexcept {
    TaskHdl schedulerHdl = SchedulerTaskProc(mainProc,  std::forward<Args>(args) ... );
    gKernel.schedulerTaskHdl = schedulerHdl;
    co_await RunSchedulerTask();
    DestroySchedulerTask(schedulerHdl);
    DebugTaskCount();
    co_return;
}

inline void InitialSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
    TaskContext* promise = &hdl.promise();
    assert(promise->state == TaskState::CREATED);
    assert(IsLinkDetached(&promise->waitLink));
    CheckInvariants();
    ++gKernel.taskCount; EnqueueLink(&gKernel.taskList, &promise->taskListLink);
    ++gKernel.readyCount; EnqueueLink(&gKernel.readyList, &promise->waitLink); promise->state = TaskState::READY;
    assert(promise->state == TaskState::READY);
    assert(!IsLinkDetached(&promise->waitLink));
    CheckInvariants();
    internal::DebugTaskCount();
}

inline TaskHdl FinalSuspendTaskOp::await_suspend(TaskHdl hdl) const noexcept {
    TaskContext* ctx = &hdl.promise();
    assert(gKernel.currentTaskHdl == hdl);
    assert(ctx->state == TaskState::RUNNING);
    assert(IsLinkDetached(&ctx->waitLink));
    CheckInvariants();
    ctx->state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    EnqueueLink(&gKernel.zombieList, &ctx->waitLink);
    ClearTaskHdl(&gKernel.currentTaskHdl);
    CheckInvariants();
    return ScheduleNextTask();
}

inline void DebugTaskCount() noexcept {
    if constexpr (TRACE_DEBUG_CODE) {
        int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
        std::print("----------------:--------\n");
        std::print("Task       count: {}\n", gKernel.taskCount);
        std::print("----------------:--------\n");
        std::print("Running    count: {}\n", running_count);
        std::print("Ready      count: {}\n", gKernel.readyCount);
        std::print("Waiting    count: {}\n", gKernel.waitingCount);
        std::print("IO waiting count: {}\n", gKernel.ioWaitingCount);
        std::print("Zombie     count: {}\n", gKernel.zombieCount);
        std::print("----------------:--------\n");
    }
}

inline void DoCheckTaskCountInvariant() noexcept {
    int running_count = gKernel.currentTaskHdl != TaskHdl() ? 1 : 0;
    bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount;
    if (!condition) { DebugTaskCount(); abort(); }
}

inline void CheckTaskCountInvariant() noexcept { if constexpr (IS_DEBUG_MODE) { DoCheckTaskCountInvariant(); } }

inline void CheckInvariants() noexcept { if constexpr (IS_DEBUG_MODE) { DoCheckTaskCountInvariant(); } }

inline constexpr TaskHdl JoinTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    TaskContext* currentTaskCtx = &currentTaskHdl.promise();
    assert(currentTaskCtx->state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentTaskCtx->waitLink));
    assert(gKernel.currentTaskHdl == currentTaskHdl);
    CheckInvariants();
    TaskContext* joinedTaskCtx = &joinedTaskHdl.promise();                
    TaskState joinedTaskState = joinedTaskCtx->state;
    switch (joinedTaskState) {
        case TaskState::READY:
        {
            currentTaskCtx->state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();
            DebugTaskCount();
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
            currentTaskCtx->state = TaskState::WAITING;
            ++gKernel.waitingCount;
            EnqueueLink(&joinedTaskCtx->awaitingTerminationList, &currentTaskCtx->waitLink); 
            ClearTaskHdl(&gKernel.currentTaskHdl);
            CheckInvariants();
            DebugTaskCount();
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
            abort();
        }
    }
}

inline TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
    assert(gKernel.currentTaskHdl);
    TaskContext* currentPromise = &currentTask.promise();
    if constexpr (IS_DEBUG_MODE) {
        assert(gKernel.currentTaskHdl == currentTask);
        assert(currentPromise->state == TaskState::RUNNING);
        assert(IsLinkDetached(&currentPromise->waitLink));
        CheckInvariants();
    }
    currentPromise->state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
    ClearTaskHdl(&gKernel.currentTaskHdl);
    CheckInvariants();
    return ScheduleNextTask();
}

inline TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    assert(gKernel.currentTaskHdl == currentTaskHdl);
    TaskContext* currentPromise = &gKernel.currentTaskHdl.promise();
    assert(IsLinkDetached(&currentPromise->waitLink));
    assert(currentPromise->state == TaskState::RUNNING);
    CheckInvariants();
    currentPromise->state = TaskState::READY;
    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &currentPromise->waitLink);
    ClearTaskHdl(&gKernel.currentTaskHdl);
    CheckInvariants();
    TaskContext* promise = &hdl.promise();
    promise->state = TaskState::RUNNING;
    DetachLink(&promise->waitLink);
    --gKernel.readyCount;
    gKernel.currentTaskHdl = hdl;
    CheckInvariants();
    assert(gKernel.currentTaskHdl);
    return hdl;
}

} } // namespace ak::internal

namespace ak {
inline void TaskContext::return_void() noexcept {
    using namespace internal;
    CheckInvariants();
    if (IsLinkDetached(&awaitingTerminationList)) { return; }
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

template <typename... Args>
inline int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept {  
    using namespace internal;
    if (InitKernel(config) < 0) { return -1; }
    KernelTaskHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
    gKernel.kernelTask = hdl;
    hdl.resume();
    FiniKernel();
    return 0;
}
} // namespace ak



