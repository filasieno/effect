#pragma once

#include <cassert>
#include <coroutine>
#include <print>
#include <cstdint>
#include <cstring>
#include <immintrin.h>
#include "liburing.h"

#include "defs.hpp"
#include "dlist.hpp"
#include "types.hpp"

#include "ak_api.hpp"

#include "ak_impl_task.hpp"
#include "ak_impl_alloc.hpp"
#include "ak_impl_boot.hpp"
#include "ak_impl_io.hpp"
#include "ak_impl_event.hpp"

namespace ak {

// Task::AwaitTaskEffect
// ----------------------------------------------------------------------------------------------------------------

namespace priv 
{
    struct RunSchedulerTaskOp {
        constexpr bool await_ready() const noexcept;
        constexpr void await_resume() const noexcept;
        TaskHdl await_suspend(KernelTaskHdl currentTaskHdl) const noexcept;
    };

    struct TerminateSchedulerOp {
        constexpr bool await_ready() const noexcept;
        KernelTaskHdl  await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept;
    };

    struct KernelTaskPromise {
        template <typename... Args>
        KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept;
        
        void* operator new(std::size_t n) noexcept;
        void  operator delete(void* ptr, std::size_t sz);

        KernelTaskHdl  get_return_object() noexcept;
        constexpr auto initial_suspend() noexcept;
        constexpr auto final_suspend() noexcept;
        constexpr void return_void() noexcept;
        constexpr void unhandled_exception() noexcept;
    };

    struct DefineKernelTask {
        using promise_type = KernelTaskPromise;

        DefineKernelTask(const KernelTaskHdl& hdl) noexcept;
        operator KernelTaskHdl() const noexcept;

        KernelTaskHdl hdl;
    };

    
    RunSchedulerTaskOp   RunSchedulerTask() noexcept;
    TerminateSchedulerOp TerminateSchedulerTask() noexcept;
    void                 DestroySchedulerTask(TaskHdl hdl) noexcept;
}

namespace priv {


    inline constexpr bool RunSchedulerTaskOp::await_ready() const noexcept { 
        return false; 
    }

    inline constexpr void RunSchedulerTaskOp::await_resume() const noexcept {}

    inline TaskHdl RunSchedulerTaskOp::await_suspend(KernelTaskHdl currentTaskHdl) const noexcept {
        using namespace priv;

        (void)currentTaskHdl;
        TaskContext& schedulerPromise = gKernel.schedulerTaskHdl.promise();

        // Check expected state post scheduler construction

        assert(gKernel.taskCount == 1);
        assert(gKernel.readyCount == 1);
        assert(schedulerPromise.state == TaskState::READY);
        assert(!IsLinkDetached(&schedulerPromise.waitLink));
        assert(gKernel.currentTaskHdl == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
        schedulerPromise.state = TaskState::RUNNING;
        DetachLink(&schedulerPromise.waitLink);
        --gKernel.readyCount;

        // Check expected state post task system bootstrap
        CheckInvariants();
        return gKernel.schedulerTaskHdl;
    }

    inline RunSchedulerTaskOp RunSchedulerTask() noexcept {
        return RunSchedulerTaskOp{};
    }

    inline constexpr bool TerminateSchedulerOp::await_ready() const noexcept { 
        return false; 
    }

    inline KernelTaskHdl TerminateSchedulerOp::await_suspend(TaskHdl hdl) const noexcept {
        using namespace priv;

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

    inline constexpr void TerminateSchedulerOp::await_resume() const noexcept {}

    inline TerminateSchedulerOp TerminateSchedulerTask() noexcept {
        return {};
    }

    inline void DestroySchedulerTask(TaskHdl hdl) noexcept {
        using namespace priv;
        TaskContext* promise = &hdl.promise();

        // Remove from Task list
        DetachLink(&promise->taskListLink);
        --gKernel.taskCount;

        // Remove from Zombie List
        DetachLink(&promise->waitLink);
        --gKernel.zombieCount;

        promise->state = TaskState::DELETING; //TODO: double check
        hdl.destroy();
    }

    template <typename... Args>
    inline KernelTaskPromise::KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept {}

    inline void* KernelTaskPromise::operator new(std::size_t n) noexcept {
        void* mem = std::malloc(n);
        if (!mem) return nullptr;
        return mem;
    }

    inline void KernelTaskPromise::operator delete(void* ptr, std::size_t sz) {
        (void)sz;
        std::free(ptr);
    }

    inline KernelTaskHdl KernelTaskPromise::get_return_object() noexcept { 
        return KernelTaskHdl::from_promise(*this); 
    }

    inline constexpr auto KernelTaskPromise::initial_suspend() noexcept { 
        return std::suspend_always {}; 
    }

    inline constexpr auto KernelTaskPromise::final_suspend() noexcept { 
        return std::suspend_never {}; 
    }

    inline constexpr void KernelTaskPromise::return_void() noexcept {}

    inline constexpr void KernelTaskPromise::unhandled_exception() noexcept { 
        assert(false); 
    }

    inline DefineKernelTask::DefineKernelTask(const KernelTaskHdl& hdl) noexcept : hdl(hdl) {}

    inline DefineKernelTask::operator KernelTaskHdl() const noexcept { 
        return hdl; 
    }

    /// \brief Schedules the next task
    /// 
    /// Used in Operations to schedule the next task.
    /// Assumes that the current task has been already suspended (moved to READY, WAITING, IO_WAITING, ...)
    ///
    /// \return the next Task to be resumed
    /// \internal
    inline TaskHdl ScheduleNextTask() noexcept {
        using namespace priv;

        // If we have a ready task, resume it
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
                // Submit Ready IO Operations
                if (ready > 0) {
                    int ret = io_uring_submit(&gKernel.ioRing);
                    if (ret < 0) {
                        std::print("io_uring_submit failed\n");
                        fflush(stdout);
                        abort();
                    }
                }

                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
                
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
                assert(zombiePromise.state == TaskState::ZOMBIE);

                // Remove from zombie list
                --gKernel.zombieCount;
                DetachLink(&zombiePromise.waitLink);

                // Remove from task list
                DetachLink(&zombiePromise.taskListLink);
                --gKernel.taskCount;

                // Delete
                zombiePromise.state = TaskState::DELETING;
                TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
                zombieTaskHdl.destroy();

                DebugTaskCount();
            }

            if (gKernel.readyCount == 0) {
                abort();
            }
        }
        // unreachable
        abort();
    }

    template <typename... Args>
    inline DefineTask SchedulerTaskProc(DefineTask(*mainProc)(Args ...) noexcept, Args... args) noexcept {
        using namespace priv;

        TaskHdl mainTask = mainProc(args...);
        assert(!mainTask.done());
        assert(GetTaskState(mainTask) == TaskState::READY);

        while (true) {
            // Sumbit IO operations
            unsigned ready = io_uring_sq_ready(&gKernel.ioRing);
            if (ready > 0) {
                int ret = io_uring_submit(&gKernel.ioRing);
                if (ret < 0) {
                    std::print("io_uring_submit failed\n");
                    fflush(stdout);
                    abort();
                }
            }

            // If we have a ready task, resume it
            if (gKernel.readyCount > 0) {
                DLink* nextNode = gKernel.readyList.prev;
                TaskContext* nextPromise = GetLinkedTaskContext(nextNode);
                TaskHdl nextTask = TaskHdl::from_promise(*nextPromise);
                assert(nextTask != gKernel.schedulerTaskHdl);
                co_await ResumeTaskOp(nextTask);
                assert(gKernel.currentTaskHdl);
                continue;
            }

            // Zombie bashing
            while (gKernel.zombieCount > 0) {
                DebugTaskCount();

                DLink* zombieNode = DequeueLink(&gKernel.zombieList);
                TaskContext& zombiePromise = *GetLinkedTaskContext(zombieNode);
                assert(zombiePromise.state == TaskState::ZOMBIE);

                // Remove from zombie list
                --gKernel.zombieCount;
                DetachLink(&zombiePromise.waitLink);

                // Remove from task list
                DetachLink(&zombiePromise.taskListLink);
                --gKernel.taskCount;

                // Delete
                zombiePromise.state = TaskState::DELETING;
                TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
                zombieTaskHdl.destroy();

                DebugTaskCount();
            }

            bool waitingCC = gKernel.ioWaitingCount;
            if (waitingCC) {
                // Process all available completions
                struct io_uring_cqe *cqe;
                unsigned head;
                unsigned completed = 0;
                io_uring_for_each_cqe(&gKernel.ioRing, head, cqe) {
                    // Return Result to the target Awaitable 
                    TaskContext* ctx = (TaskContext*) io_uring_cqe_get_data(cqe);
                    assert(ctx->state == TaskState::IO_WAITING);

                    // Move the target task from IO_WAITING to READY
                    --gKernel.ioWaitingCount;
                    ctx->state = TaskState::READY;
                    ++gKernel.readyCount;
                    EnqueueLink(&gKernel.readyList, &ctx->waitLink);
                    
                    // Complete operation
                    ctx->ioResult = cqe->res;
                    --ctx->enqueuedIO;
                    ++completed;
                }
                // Mark all as seen
                io_uring_cq_advance(&gKernel.ioRing, completed);
            }

            if (gKernel.readyCount == 0 && gKernel.ioWaitingCount == 0) {
                break;
            }
        }
        co_await TerminateSchedulerTask();

        assert(false); // Unreachale
        co_return;
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



    inline int InitKernel(KernelConfig* config) noexcept {
        using namespace priv;
        
        if (InitAllocTable(&gKernel.allocTable, config->mem, config->memSize) != 0) {
            return -1;
        }

        int res = io_uring_queue_init(config->ioEntryCount, &gKernel.ioRing, 0);
        if (res < 0) {
            std::print("io_uring_queue_init failed\n");
            return -1;
        }

        gKernel.mem = config->mem;
        gKernel.memSize = config->memSize;
        gKernel.taskCount = 0;
        gKernel.readyCount = 0;
        gKernel.waitingCount = 0;
        gKernel.ioWaitingCount = 0;
        gKernel.zombieCount = 0;
        gKernel.interrupted = 0;

        ClearTaskHdl(&gKernel.currentTaskHdl);
        ClearTaskHdl(&gKernel.schedulerTaskHdl);

        InitLink(&gKernel.zombieList);
        InitLink(&gKernel.readyList);
        InitLink(&gKernel.taskList);
        
        return 0;
    }

    inline void FiniKernel() noexcept {
        io_uring_queue_exit(&gKernel.ioRing);
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
        if (!condition) {
            DebugTaskCount();
            abort();
        }
    }

    inline void CheckTaskCountInvariant() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            DoCheckTaskCountInvariant();
        }
    }

    inline void CheckInvariants() noexcept {
        if constexpr (IS_DEBUG_MODE) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }
    }






}

/// \brief Runs the main task
/// \param UserProc the user's main task
/// \return 0 on success
/// \ingroup Kernel




// -----------------------------------------------------------------------------
// DebugAPI 
// -----------------------------------------------------------------------------

inline void DebugIOURingFeatures(const unsigned int features) {
    std::print("IO uring features:\n");
    if (features & IORING_FEAT_SINGLE_MMAP)     std::print("  SINGLE_MMAP\n");
    if (features & IORING_FEAT_NODROP)          std::print("  NODROP\n");
    if (features & IORING_FEAT_SUBMIT_STABLE)   std::print("  SUBMIT_STABLE\n");
    if (features & IORING_FEAT_RW_CUR_POS)      std::print("  RW_CUR_POS\n");
    if (features & IORING_FEAT_CUR_PERSONALITY) std::print("  CUR_PERSONALITY\n");
    if (features & IORING_FEAT_FAST_POLL)       std::print("  FAST_POLL\n");
    if (features & IORING_FEAT_POLL_32BITS)     std::print("  POLL_32BITS\n");
    if (features & IORING_FEAT_SQPOLL_NONFIXED) std::print("  SQPOLL_NONFIXED\n");
    if (features & IORING_FEAT_EXT_ARG)         std::print("  EXT_ARG\n");
    if (features & IORING_FEAT_NATIVE_WORKERS)  std::print("  NATIVE_WORKERS\n");
}

inline void DebugIOURingSetupFlags(const unsigned int flags) {
    std::print("IO uring flags:\n");
    if (flags & IORING_SETUP_IOPOLL)    std::print("  IOPOLL\n");
    if (flags & IORING_SETUP_SQPOLL)    std::print("  SQPOLL\n");
    if (flags & IORING_SETUP_SQ_AFF)    std::print("  SQ_AFF\n");
    if (flags & IORING_SETUP_CQSIZE)    std::print("  CQSIZE\n");
    if (flags & IORING_SETUP_CLAMP)     std::print("  CLAMP\n");
    if (flags & IORING_SETUP_ATTACH_WQ) std::print("  ATTACH_WQ\n");
}

inline void DebugIOURingParams(const io_uring_params* p) {
    std::print("IO uring parameters:\n");
    
    // Main parameters
    std::print("Main Configuration:\n");
    std::print("  sq_entries: {}\n", p->sq_entries);
    std::print("  cq_entries: {}\n", p->cq_entries);
    std::print("  sq_thread_cpu: {}\n", p->sq_thread_cpu);
    std::print("  sq_thread_idle: {}\n", p->sq_thread_idle);
    std::print("  wq_fd: {}\n", p->wq_fd);

    // Print flags
    DebugIOURingSetupFlags(p->flags);

    // Print features
    DebugIOURingFeatures(p->features);

    // Submission Queue Offsets

    std::print("Submission Queue Offsets:\n");
    std::print("  head: {}\n", p->sq_off.head);
    std::print("  tail: {}\n", p->sq_off.tail);
    std::print("  ring_mask: {}\n", p->sq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->sq_off.ring_entries);
    std::print("  flags: {}\n", p->sq_off.flags);
    std::print("  dropped: {}\n", p->sq_off.dropped);
    std::print("  array: {}\n", p->sq_off.array);

    // Completion Queue Offsets

    std::print("Completion Queue Offsets:\n");
    std::print("  head: {}\n", p->cq_off.head);
    std::print("  tail: {}\n", p->cq_off.tail);
    std::print("  ring_mask: {}\n", p->cq_off.ring_mask);
    std::print("  ring_entries: {}\n", p->cq_off.ring_entries);
    std::print("  overflow: {}\n", p->cq_off.overflow);
    std::print("  cqes: {}\n", p->cq_off.cqes);
    std::print("  flags: {}\n", p->cq_off.flags);
    std::print("\n");
    std::fflush(stdout);
}

// -----------------------------------------------------------------------------
// IO Operators
// -----------------------------------------------------------------------------

inline int GetCurrentTaskEnqueuedIOOps() noexcept {
    return GetTaskContext()->enqueuedIO;
}


#include "ak_impl_alloc.hpp"

inline void DebugDumpAllocTable(priv::AllocTable* at) noexcept {

    // Basic layout and sizes
    std::print("AllocTable: {}\n", (void*)at);
    
    std::print("  heapBegin        : {}\n", (void*)at->heapBegin);
    std::print("  heapEnd          : {}; size: {}\n", (void*)at->heapEnd, (intptr_t)(at->heapEnd - at->heapBegin));
    std::print("  memBegin         : {}\n", (void*)at->memBegin);
    std::print("  memEnd           : {}; size: {}\n", (void*)at->memEnd, (intptr_t)(at->memEnd - at->memBegin));
    std::print("  memSize          : {}\n", at->memSize);
    std::print("  usedMemSize      : {}\n", at->usedMemSize);
    std::print("  freeMemSize      : {}\n", at->freeMemSize);

    // Sentinels and wild/large tracking (addresses only; do not dereference)
    std::print("  Key Offsets:\n");
    std::print("    Begin sentinel offset: {}\n", (intptr_t)at->beginSentinel - (intptr_t)at->memBegin);
    std::print("    Wild  block    offset: {}\n", (intptr_t)at->wildBlock - (intptr_t)at->memBegin);
    std::print("    LB    sentinel offset: {}\n", (intptr_t)at->largeBlockSentinel - (intptr_t)at->memBegin);
    std::print("    End   sentinel offset: {}\n", (intptr_t)at->endSentinel - (intptr_t)at->memBegin);

    // Free list availability mask as a bit array (256 bits)
    std::print("  FreeListbinMask:");
    alignas(32) uint64_t lanesPrint[4] = {0,0,0,0};
    static_assert(sizeof(lanesPrint) == 32, "lanesPrint must be 256 bits");
    std::memcpy(lanesPrint, &at->freeListbinMask, 32);
    for (unsigned i = 0; i < 256; i++) {
        if (i % 64 == 0) std::print("\n    ");
        unsigned lane = i >> 6;
        unsigned bit  = i & 63u;
        std::print("{}", (lanesPrint[lane] >> bit) & 1ull);
    }
        std::print("\n");

    // Optional per-bin size accounting
    
    std::print("  FreeListBinsSizes begin\n");
    for (unsigned i = 0; i < 254; ++i) {
        unsigned cc = at->freeListBinsCount[i];
        if (cc == 0) continue;
        std::print("    {:>3} bytes class  : {}\n", i * 32, cc);
    }
    std::print("    medium class (254) : {}\n", at->freeListBinsCount[254]);
    std::print("    wild class   (255) : {}\n", at->freeListBinsCount[255]);
    std::print("  FreeListBinsSizes end\n");
    

    // Aggregate statistics
    // std::print("maxFreeBlockSize: {}\n", at->maxFreeBlockSize);
    // std::print("totalAllocCount: {}\n", at->totalAllocCount);
    // std::print("totalFreeCount: {}\n", at->totalFreeCount);
    // std::print("totalReallocCount: {}\n", at->totalReallocCount);
    // std::print("totalSplitCount: {}\n", at->totalSplitCount);
    // std::print("totalMergeCount: {}\n", at->totalMergeCount);
    // std::print("totalReuseCount: {}\n", at->totalReuseCount);
            
    std::print("\n");
}

namespace priv {

    static inline void SetAllocFreeBinBit(AllocTable* at, unsigned binIdx) {       
        assert(at != nullptr);
        assert(binIdx < 256);
        const unsigned lane = binIdx >> 6;         // 0..3
        const unsigned bit  = binIdx & 63u;        // 0..63
        alignas(32) uint64_t lanes[4];
        std::memcpy(lanes, &at->freeListbinMask, 32);
        lanes[lane] |= (1ull << bit);
        std::memcpy(&at->freeListbinMask, lanes, 32);
    }

    static inline bool GetAllocFreeBinBit(AllocTable* at, unsigned binIdx) {       
        assert(at != nullptr);
        assert(binIdx < 256);
        const unsigned lane = binIdx >> 6;         // 0..3
        const unsigned bit  = binIdx & 63u;        // 0..63
        alignas(32) uint64_t lanes[4];
        std::memcpy(lanes, &at->freeListbinMask, 32);
        return ((lanes[lane] >> bit) & 1ull) != 0ull;
    }

    static inline void ClearAllocFreeBinBit(AllocTable* at, unsigned binIdx) {       
        assert(at != nullptr);
        assert(binIdx < 256);
        const unsigned lane = binIdx >> 6;         // 0..3
        const unsigned bit  = binIdx & 63u;        // 0..63
        alignas(32) uint64_t lanes[4];
        std::memcpy(lanes, &at->freeListbinMask, 32);
        lanes[lane] &= ~(1ull << bit);
        std::memcpy(&at->freeListbinMask, lanes, 32);
    }

    inline int InitAllocTable(AllocTable* at, void* mem, Size size) noexcept {
        
        
        constexpr U64 SENTINEL_SIZE = sizeof(FreeAllocHeader);

        assert(mem != nullptr);
        assert(size >= 4096);

        memset((void*)at, 0, sizeof(AllocTable));
        
        // Establish heap boundaries
        char* heapBegin = (char*)(mem);
        char* heapEnd   = heapBegin + size;

        // // Align start up to 32 and end down to 32 to keep all blocks 32B-multiples
        U64 alignedBegin = ((U64)heapBegin + SENTINEL_SIZE) & ~31ull;
        U64 alignedEnd   = ((U64)heapEnd   - SENTINEL_SIZE) & ~31ull;

        at->heapBegin = heapBegin;
        at->heapEnd   = heapEnd;
        at->memBegin  = (char*)alignedBegin;
        at->memEnd    = (char*)alignedEnd;
        at->memSize   = (Size)(at->memEnd - at->memBegin);

        // Addresses
        // Layout: [BeginSentinel] ... blocks ... [LargeBlockSentinel] ... largeBlocks ... [EndSentinel]
        FreeAllocHeader* beginSentinel      = (FreeAllocHeader*)alignedBegin;
        FreeAllocHeader* wildBlock          = (FreeAllocHeader*)((char*)beginSentinel + SENTINEL_SIZE);
        FreeAllocHeader* endSentinel        = (FreeAllocHeader*)((char*)alignedEnd - SENTINEL_SIZE); 
        FreeAllocHeader* largeBlockSentinel = (FreeAllocHeader*)((char*)endSentinel - SENTINEL_SIZE);
        InitLink(&wildBlock->freeListLink);
        
        // Check alignments
        assert(((U64)beginSentinel      & 31ull) == 0ull);
        assert(((U64)wildBlock          & 31ull) == 0ull);
        assert(((U64)endSentinel        & 31ull) == 0ull);
        assert(((U64)largeBlockSentinel & 31ull) == 0ull);
        
        at->beginSentinel      = beginSentinel;
        at->wildBlock          = wildBlock;
        at->endSentinel        = endSentinel;
        at->largeBlockSentinel = largeBlockSentinel;
        
        beginSentinel->thisSize.size       = (U64)SENTINEL_SIZE;
        beginSentinel->thisSize.state      = (U32)AllocState::BEGIN_SENTINEL;
        wildBlock->thisSize.size           = (U64)((U64)largeBlockSentinel - (U64)wildBlock);
        wildBlock->thisSize.state          = (U32)AllocState::WILD_BLOCK;
        largeBlockSentinel->thisSize.size  = (U64)SENTINEL_SIZE;
        largeBlockSentinel->thisSize.state = (U32)AllocState::LARGE_BLOCK_SENTINEL;
        endSentinel->thisSize.size         = (U64)SENTINEL_SIZE;
        endSentinel->thisSize.state        = (U32)AllocState::END_SENTINEL;
        wildBlock->prevSize                = beginSentinel->thisSize;
        largeBlockSentinel->prevSize       = wildBlock->thisSize;
        endSentinel->prevSize              = largeBlockSentinel->thisSize;
        at->freeMemSize                    = wildBlock->thisSize.size;

        for (int i = 0; i < 256; ++i) {
            InitLink(&at->freeListBins[i]);
        }

        SetAllocFreeBinBit(at, 255);
        DLink* freeList = &at->freeListBins[255];
        InitLink(&wildBlock->freeListLink);
        InsertNextLink(freeList, &wildBlock->freeListLink);
        at->freeListBinsCount[255] = 1;

        return 0;
    }

}

namespace priv {
    constexpr const char* DEBUG_ALLOC_COLOR_RESET  = "\033[0m";
    constexpr const char* DEBUG_ALLOC_COLOR_WHITE  = "\033[37m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_GREEN  = "\033[1;32m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_YELLOW = "\033[1;33m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_CYAN   = "\033[36m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_MAG    = "\033[35m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_RED    = "\033[1;31m"; 
    constexpr const char* DEBUG_ALLOC_COLOR_HDR    = "\033[36m"; 

    inline static AllocHeader* NextHeaderPtr(AllocHeader* h) {
        size_t sz = (size_t)h->thisSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((char*)h + sz);
    }

    inline static AllocHeader* PrevHeaderPtr(AllocHeader* h) {
        size_t sz = (size_t)h->prevSize.size;
        if (sz == 0) return h;
        return (AllocHeader*)((char*)h - sz);
    }
    
    inline const char* StateText(AllocState s) {
        switch (s) {
            case AllocState::USED:                 return "USED";
            case AllocState::FREE:                 return "FREE";
            case AllocState::WILD_BLOCK:           return "WILD";
            case AllocState::BEGIN_SENTINEL:       return "SENTINEL B";
            case AllocState::LARGE_BLOCK_SENTINEL: return "SENTINEL L";
            case AllocState::END_SENTINEL:         return "SENTINEL E";
            default:                               return "INVALID";
        }
    }
    
    static inline constexpr const char* StateColor(AllocState s) {
        switch (s) {
            case AllocState::USED:               
                return DEBUG_ALLOC_COLOR_CYAN;
            case AllocState::FREE:   
            case AllocState::WILD_BLOCK: 
                return DEBUG_ALLOC_COLOR_GREEN;
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL: 
                return DEBUG_ALLOC_COLOR_YELLOW;
            case AllocState::INVALID: 
                return DEBUG_ALLOC_COLOR_RED;
            default: 
                return DEBUG_ALLOC_COLOR_RESET;
        }
    }
    
    static inline unsigned GetSmallBinIndexFromSize(uint64_t sz) {
        if (sz < 32) return 0u;
        if (sz <= 32ull * 254ull) return (unsigned)(sz / 32ull) - 1u;
        return 254u; // 254 = medium, 255 = wild
    }

    static inline void PrintRun(const char* s, int n, const char* color = DEBUG_ALLOC_COLOR_WHITE) {
        for (int i = 0; i < n; ++i) std::print("{}{}", color, s);
    }

    inline static unsigned GetFreeListBinIndex(const AllocHeader* h) {
        switch ((AllocState)h->thisSize.state) {
            case AllocState::WILD_BLOCK:
                return 255;
            case AllocState::FREE: 
            {
                Size sz = h->thisSize.size;
                if (sz >= 254ull * 32ull) return 254;
                else return (unsigned)(sz / 32ull);
            }
            case AllocState::INVALID:
            case AllocState::USED:
            case AllocState::BEGIN_SENTINEL:
            case AllocState::LARGE_BLOCK_SENTINEL:
            case AllocState::END_SENTINEL:
            default:
            {
                return 256;
            }
        }
    }

    // Fixed column widths (constants) in requested order
    constexpr int DEBUG_COL_W_OFF     = 18; // 0x + 16 hex
    constexpr int DEBUG_COL_W_SIZE    = 12;
    constexpr int DEBUG_COL_W_STATE   = 10;
    constexpr int DEBUG_COL_W_PSIZE   = 12;
    constexpr int DEBUG_COL_W_PSTATE  = 10;
    constexpr int DEBUG_COL_W_FL_PREV = 18;
    constexpr int DEBUG_COL_W_FL_NEXT = 18;

    static inline void PrintTopBorder() {
        std::print("{}┌{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┬{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┐{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintHeaderSeparator() {
        std::print("{}├{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┼{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┤{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintBottomBorder() {
        std::print("{}└{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_OFF + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_SIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_STATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSIZE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_PSTATE + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_PREV + 2);
        std::print("{}┴{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        PrintRun("─", DEBUG_COL_W_FL_NEXT + 2);
        std::print("{}┘{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintHeader() {
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "Offset");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "Size");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "State");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevSize");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} "  , DEBUG_ALLOC_COLOR_HDR,   "PrevState");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListPrev");
        std::print("{}│{}"       , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} "  , DEBUG_ALLOC_COLOR_HDR,   "FreeListNext");
        std::print("{}│{}\n"     , DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }

    static inline void PrintRow(const AllocHeader* h) {
        const AllocTable* at = &gKernel.allocTable;
        uintptr_t beginAddr = (uintptr_t)at->beginSentinel;
        uintptr_t off = (uintptr_t)h - beginAddr;
        uint64_t  sz  = (uint64_t)h->thisSize.size;
        uint64_t  psz = (uint64_t)h->prevSize.size;
        AllocState st = (AllocState)h->thisSize.state;
        AllocState pst = (AllocState)h->prevSize.state;

        const char* stateText = StateText(st);
        const char* previousStateText = StateText(pst);
        const char* stateColor = StateColor(st);

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<18} ", stateColor, (unsigned long long)off);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)sz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, stateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<12} ", stateColor, (unsigned long long)psz);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        std::print("{} {:<10} ", stateColor, previousStateText);
        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
        
        // Print FreeListPrev
        if (h->thisSize.state == (U32)AllocState::FREE) {
            std::print("{} {:<18} ", stateColor, "TODO");
        } else if (h->thisSize.state == (U32)AllocState::WILD_BLOCK) {
            FreeAllocHeader* freeBlock = (FreeAllocHeader*)h;
            if (freeBlock->freeListLink.prev == &at->freeListBins[255]) {
                std::print("{} {:<18} ", DEBUG_ALLOC_COLOR_GREEN, "WILD LIST");
            } else {
                std::print("{} {:<18} ", DEBUG_ALLOC_COLOR_RED, "INVALID");
            }
        } else {
            std::print("{} {:<18} ", stateColor, "");
        }

        std::print("{}│{}", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);

        // Print FreeList Next
        if (h->thisSize.state == (U32)AllocState::FREE) {
            std::print("{} {:<18} ", stateColor, "TODO");
        } else if (h->thisSize.state == (U32)AllocState::WILD_BLOCK) {
            FreeAllocHeader* freeBlock = (FreeAllocHeader*)h;
            if (freeBlock->freeListLink.next == &at->freeListBins[255]) {
                std::print("{} {:<18} ", DEBUG_ALLOC_COLOR_GREEN, "WILD LIST");
            } else {
                std::print("{} {:<18} ", DEBUG_ALLOC_COLOR_RED, "INVALID");
            }
        } else {
            std::print("{} {:<18} ", stateColor, "");
        }


        std::print("{}│{}\n", DEBUG_ALLOC_COLOR_WHITE, DEBUG_ALLOC_COLOR_RESET);
    }
    
    /// \brief Find the smallest free list that can store the allocSize
    /// 
    /// \param allocSize The size of the allocation
    /// \param bitField A pointer to a 64 byte aligned bit field
    /// \return The index of the smallest free list that can store the allocSize
    /// \pre AVX2 is available
    /// \pre bitField is 64 byte aligned
    static inline int FindFreeListBucket(Size allocSize, char* bitField) noexcept {
        assert(bitField != nullptr);
        assert(((uintptr_t)bitField % 64ull) == 0ull);

        // Compute the starting bin index (ceil(allocSize/32) - 1), clamped to [0,255]
        unsigned requiredBin = 0u;
        if (allocSize != 0) {
            requiredBin = (unsigned)((allocSize - 1u) >> 5); // floor((allocSize-1)/32)
        }
        if (requiredBin > 255u) requiredBin = 255u;

#if defined(__AVX2__)
        // AVX2 fast path (no runtime feature check)
        // Build a byte-granular mask: zero bytes < requiredByte, keep bytes >= requiredByte;
        // additionally mask bits < bitInByte in the requiredByte itself
        const unsigned requiredByte = requiredBin >> 3;   // 0..31
        const unsigned bitInByteReq = requiredBin & 7u;   // 0..7

        // Precomputed 0..31 index vector (one-time constant)
        alignas(32) static const unsigned char INDEX_0_31[32] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        };

        const __m256i availability = _mm256_load_si256((const __m256i*)bitField);
        const __m256i idx = _mm256_load_si256((const __m256i*)INDEX_0_31);
        const __m256i reqByteVec = _mm256_set1_epi8((char)requiredByte);
        const __m256i allOnes    = _mm256_set1_epi8((char)-1);

        // geMask: 0xFF where idx >= requiredByte, 0x00 otherwise (one compare + andnot)
        const __m256i ltMask = _mm256_cmpgt_epi8(reqByteVec, idx); // 0xFF where req > idx (i.e., idx < req)
        const __m256i geMask = _mm256_andnot_si256(ltMask, allOnes);

        // Apply byte-level mask and also clear bits below bitInByteReq in the required byte
        __m256i masked = _mm256_and_si256(availability, geMask);
        const __m256i eqMask = _mm256_cmpeq_epi8(idx, reqByteVec);
        const __m256i firstByteMaskVec = _mm256_set1_epi8((char)(unsigned char)(0xFFu << bitInByteReq));
        const __m256i onlyEqLane       = _mm256_and_si256(masked, eqMask);
        const __m256i maskedEqLane     = _mm256_and_si256(onlyEqLane, firstByteMaskVec);
        const __m256i otherLanes       = _mm256_andnot_si256(eqMask, masked);
        masked = _mm256_or_si256(otherLanes, maskedEqLane);

        // Find the lowest set bit using movemask on zero-compare
        const __m256i zero = _mm256_setzero_si256();
        const __m256i is_zero = _mm256_cmpeq_epi8(masked, zero);
        const unsigned non_zero_mask = ~static_cast<unsigned>(_mm256_movemask_epi8(is_zero));
        if (non_zero_mask == 0u) {
            return 255;
        }

        const int byte_idx = __builtin_ctz(non_zero_mask);
        // Read the target byte directly from the original bitfield and apply the per-byte bit mask for the first lane only (branchless)
        const unsigned char* bytes_src = (const unsigned char*)bitField;
        unsigned char byte_val = bytes_src[byte_idx];
        const unsigned char firstByteMaskScalar = (unsigned char)(0xFFu << bitInByteReq);
        const unsigned char sameMask = (unsigned char)-(byte_idx == (int)requiredByte); // 0xFF if same, 0x00 otherwise
        byte_val &= (unsigned char)((sameMask & firstByteMaskScalar) | (~sameMask));
        const int bit_in_byte = __builtin_ctz((unsigned)byte_val);
        return byte_idx * 8 + bit_in_byte;
#else
        // Fallback: branchless, fully unrolled scan of 4x64-bit words
        const uint64_t* words = (const uint64_t*)bitField; // 4 x 64-bit words
        const unsigned wordIdx = requiredBin >> 6;        // 0..3
        const unsigned bitInWord = requiredBin & 63u;    // 0..63

        const uint64_t startMask = ~0ull << bitInWord;   // valid when selecting the starting word

        // Enable masks for words >= word_idx (0xFFFFFFFFFFFFFFFF or 0x0)
        const uint64_t en0 = (uint64_t)-(0u >= wordIdx);
        const uint64_t en1 = (uint64_t)-(1u >= wordIdx);
        const uint64_t en2 = (uint64_t)-(2u >= wordIdx);
        const uint64_t en3 = (uint64_t)-(3u >= wordIdx);

        // Equal masks for exactly the starting word
        const uint64_t eq0 = (uint64_t)-(0u == wordIdx);
        const uint64_t eq1 = (uint64_t)-(1u == wordIdx);
        const uint64_t eq2 = (uint64_t)-(2u == wordIdx);
        const uint64_t eq3 = (uint64_t)-(3u == wordIdx);

        // Per-word effective masks: for starting word use startMask, otherwise ~0ull; then gate with enable mask
        const uint64_t mask0 = en0 & ((eq0 & startMask) | (~eq0 & ~0ull));
        const uint64_t mask1 = en1 & ((eq1 & startMask) | (~eq1 & ~0ull));
        const uint64_t mask2 = en2 & ((eq2 & startMask) | (~eq2 & ~0ull));
        const uint64_t mask3 = en3 & ((eq3 & startMask) | (~eq3 & ~0ull));

        const uint64_t v0 = words[0] & mask0;
        const uint64_t v1 = words[1] & mask1;
        const uint64_t v2 = words[2] & mask2;
        const uint64_t v3 = words[3] & mask3;

        const unsigned n0 = (unsigned)(v0 != 0);
        const unsigned n1 = (unsigned)(v1 != 0);
        const unsigned n2 = (unsigned)(v2 != 0);
        const unsigned n3 = (unsigned)(v3 != 0);
        const unsigned nonzero_groups = (n0) | (n1 << 1) | (n2 << 2) | (n3 << 3);
        if (nonzero_groups == 0u) {
            return 255;
        }

        const unsigned group = (unsigned)__builtin_ctz(nonzero_groups); // 0..3

        // Branchless selection of the first nonzero word
        const uint64_t sel0 = (uint64_t)-(group == 0u);
        const uint64_t sel1 = (uint64_t)-(group == 1u);
        const uint64_t sel2 = (uint64_t)-(group == 2u);
        const uint64_t sel3 = (uint64_t)-(group == 3u);
        const uint64_t vv = (v0 & sel0) | (v1 & sel1) | (v2 & sel2) | (v3 & sel3);

        const unsigned bit_in_word_first = (unsigned)__builtin_ctzll(vv);
        return (int)(group * 64u + bit_in_word_first);
#endif

    }

    static inline bool GetFreeListBit(char* bitField, int bucket) noexcept {
        assert(bitField != nullptr);
        assert(bucket >= 0 && bucket < 256);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(bitField);
        int byte_idx = bucket / 8;
        int bit_in_byte = bucket % 8;
        return (bytes[byte_idx] & (1u << bit_in_byte)) != 0;
    }

    static inline void SetFreeListBit(char* bitField, int bucket) noexcept {
        assert(bitField != nullptr);
        assert(bucket >= 0 && bucket < 256);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(bitField);
        int byte_idx = bucket / 8;
        int bit_in_byte = bucket % 8;
        bytes[byte_idx] |= (1u << bit_in_byte);
    }

    static inline void ClearFreeListBit(char* bitField, int bucket) noexcept {
        assert(bitField != nullptr);
        assert(bucket >= 0 && bucket < 256);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(bitField);
        int byte_idx = bucket / 8;
        int bit_in_byte = bucket % 8;
        bytes[byte_idx] &= ~(1u << bit_in_byte);
    }
}

inline void DebugPrintAllocBlocks(priv::AllocTable* at) noexcept 
{
    using namespace priv;
    
    
    PrintTopBorder();
    PrintHeader();
    PrintHeaderSeparator();
    AllocHeader* head = (AllocHeader*) at->beginSentinel;
    AllocHeader* end  = (AllocHeader*) NextHeaderPtr((AllocHeader*)at->endSentinel);
    
    for (; head != end; head = NextHeaderPtr(head)) {
        PrintRow(head);
    }

    PrintBottomBorder();
}

// Constants for allocator (add at top of allocator section, around line 212)
constexpr Size HEADER_SIZE = 16;
constexpr Size MIN_BLOCK_SIZE = 32;
constexpr Size ALIGNMENT = 32;

// In TryMalloc (replace existing function starting at 2933):
/// \brief Attempts to synchronously allocate memory from the heap.
/// 
/// Algorithm:
/// 1. Compute aligned block size: Add HEADER_SIZE and round up to ALIGNMENT.
/// 2. Find smallest available bin >= required using SIMD-accelerated search.
/// 3. For small bins (<254): Pop free block, split if larger than needed.
/// 4. For medium bin (254): First-fit search on list, split if possible.
/// 5. For wild bin (255): Split from wild block or allocate entirely if exact match.
/// 
/// Returns nullptr if no suitable block found (heap doesn't grow).
/// For async version that suspends on failure, use co_await AllocMem(size).
inline void* TryMalloc(priv::AllocTable* at, Size size) noexcept {
    using namespace priv;
    
    // Compute aligned block size
    Size maybeBlock = HEADER_SIZE + size;
    Size unaligned = maybeBlock & (ALIGNMENT - 1);
    Size requestedBlockSize = (unaligned != 0) ? maybeBlock + (ALIGNMENT - unaligned) : maybeBlock;
    assert((requestedBlockSize & (ALIGNMENT - 1)) == 0);
    assert(requestedBlockSize >= MIN_BLOCK_SIZE);
    
    // Find bin
    int binIdx = FindFreeListBucket(requestedBlockSize, (char*)&at->freeListbinMask);
    assert(GetAllocFreeBinBit(at, binIdx));
    assert(!IsLinkDetached(&at->freeListBins[binIdx]));
    assert(at->freeListBinsCount[binIdx] > 0);
    
    if (binIdx < 254) {  // Small bin: Pop and optional split
        DLink* freeStack = &at->freeListBins[binIdx];
        DLink* link = PopLink(freeStack);
        --at->freeListBinsCount[binIdx];
        if (at->freeListBinsCount[binIdx] == 0) ClearAllocFreeBinBit(at, binIdx);
        
        AllocHeader* block = (AllocHeader*)((char*)link - offsetof(FreeAllocHeader, freeListLink));
        AllocHeader* nextBlock = NextHeaderPtr(block);
        __builtin_prefetch(nextBlock, 1, 3);
        
        if constexpr (IS_DEBUG_MODE) {
            ClearLink(link);
        }

        Size blockSize = block->thisSize.size;
        
        if (blockSize == requestedBlockSize) {  // Exact match
            block->thisSize.state = (U32)AllocState::USED;
            at->freeMemSize -= requestedBlockSize;
            AllocStats* stats = &at->stats;
            ++stats->binAllocCount[binIdx];
            ++stats->binReuseCount[binIdx];
            nextBlock->prevSize.state = (U32)AllocState::USED;
            return (void*)((char*)block + HEADER_SIZE);
        } else {  // Split
            Size newFreeSize = blockSize - requestedBlockSize;
            assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
            
            FreeAllocHeader* newFree = (FreeAllocHeader*)((char*)block + requestedBlockSize);
            __builtin_prefetch(newFree, 1, 3);
            
            block->thisSize.size = requestedBlockSize;
            block->thisSize.state = (U32)AllocState::USED;
            
            newFree->thisSize.size = newFreeSize;
            newFree->thisSize.state = (U32)AllocState::FREE;
            newFree->prevSize = block->thisSize;
            
            nextBlock->prevSize = newFree->thisSize;
            
            int newBinIdx = (newFreeSize / ALIGNMENT) - 1;
            DLink* newStack = &at->freeListBins[newBinIdx];
            PushLink(newStack, &newFree->freeListLink);
            ++at->freeListBinsCount[newBinIdx];
            if (at->freeListBinsCount[newBinIdx] == 1) SetAllocFreeBinBit(at, newBinIdx);
            
            AllocStats* stats = &at->stats;
            ++stats->binAllocCount[binIdx];
            ++stats->binSplitCount[binIdx];
            ++stats->binPoolCount[newBinIdx];
            at->freeMemSize -= requestedBlockSize;
            
            return (void*)((char*)block + HEADER_SIZE);
        }
    }
    
    if (binIdx == 254) {  // Medium bin: First-fit search and split
        DLink* mediumList = &at->freeListBins[254];
        for (DLink* link = mediumList->next; link != mediumList; link = link->next) {
            FreeAllocHeader* block = (FreeAllocHeader*)((char*)link - offsetof(FreeAllocHeader, freeListLink));
            Size blockSize = block->thisSize.size;
            if (blockSize >= requestedBlockSize) {
                DetachLink(link);
                --at->freeListBinsCount[254];
                if (at->freeListBinsCount[254] == 0) ClearAllocFreeBinBit(at, 254);
                
                AllocHeader* nextBlock = NextHeaderPtr((AllocHeader*)block);
                __builtin_prefetch(nextBlock, 1, 3);
                
                if constexpr (IS_DEBUG_MODE) {
                    ClearLink(link);
                }         

                if (blockSize == requestedBlockSize) {  // Exact
                    block->thisSize.state = (U32)AllocState::USED;
                    at->freeMemSize -= requestedBlockSize;
                    AllocStats* stats = &at->stats;
                    ++stats->binAllocCount[254];
                    ++stats->binReuseCount[254];
                    nextBlock->prevSize.state = (U32)AllocState::USED;
                    return (void*)((char*)block + HEADER_SIZE);
                } else {  // Split
                    Size newFreeSize = blockSize - requestedBlockSize;
                    assert(newFreeSize >= MIN_BLOCK_SIZE && newFreeSize % ALIGNMENT == 0);
                    
                    FreeAllocHeader* newFree = (FreeAllocHeader*)((char*)block + requestedBlockSize);
                    __builtin_prefetch(newFree, 1, 3);
                    
                    block->thisSize.size = requestedBlockSize;
                    block->thisSize.state = (U32)AllocState::USED;
                    
                    newFree->thisSize.size = newFreeSize;
                    newFree->thisSize.state = (U32)AllocState::FREE;
                    newFree->prevSize = block->thisSize;
                    
                    nextBlock->prevSize = newFree->thisSize;
                    
                    // For medium remainder, push back to medium bin (254)
                    U64 newBinIdx = GetSmallBinIndexFromSize((U64)newFreeSize);
                    DLink* newStack = &at->freeListBins[newBinIdx];
                    PushLink(newStack, &newFree->freeListLink);
                    ++at->freeListBinsCount[newBinIdx];
                    SetAllocFreeBinBit(at, newBinIdx);
                    
                    AllocStats* stats = &at->stats;
                    ++stats->binAllocCount[254];
                    ++stats->binSplitCount[254];
                    ++stats->binPoolCount[newBinIdx];
                    at->freeMemSize -= requestedBlockSize;
                    
                    return (void*)((char*)block + HEADER_SIZE);
                }
            }
        }
        return nullptr;  // No fit found
    }
    
    if (binIdx == 255) {  // Wild bin
        assert(at->wildBlock != nullptr);
        DLink* freeStack = &at->freeListBins[255];
        DLink* link = &at->wildBlock->freeListLink;
        DetachLink(link);
        assert(freeStack->prev == freeStack && freeStack->next == freeStack);  // Empty after detach
        at->freeListBinsCount[255] = 0;
        ClearAllocFreeBinBit(at, 255);
        
        AllocHeader* oldWild = (AllocHeader*)((char*)link - offsetof(FreeAllocHeader, freeListLink));
        AllocHeader* nextBlock = NextHeaderPtr(oldWild);
        __builtin_prefetch(nextBlock, 1, 3);
        assert(at->wildBlock == (FreeAllocHeader*)oldWild);
        
        Size oldSize = oldWild->thisSize.size;
        
        if (requestedBlockSize >= (oldSize + 32)) return nullptr; // not enough space
        
        Size newWildSize = oldSize - requestedBlockSize;
        assert(newWildSize >= MIN_BLOCK_SIZE && newWildSize % ALIGNMENT == 0);
        
        AllocHeader* allocated = oldWild;
        allocated->thisSize.size = requestedBlockSize;
        allocated->thisSize.state = (U32)AllocState::USED;
        
        FreeAllocHeader* newWild = (FreeAllocHeader*)((char*)allocated + requestedBlockSize);
        newWild->thisSize.size = newWildSize;
        newWild->thisSize.state = (U32)AllocState::WILD_BLOCK;
        newWild->prevSize = allocated->thisSize;
        
        nextBlock->prevSize = newWild->thisSize;
        
        at->wildBlock = newWild;
        PushLink(freeStack, &newWild->freeListLink);
        ++at->freeListBinsCount[255];
        SetAllocFreeBinBit(at, 255);
        
        at->freeMemSize -= requestedBlockSize;
        AllocStats* stats = &at->stats;
        ++stats->binAllocCount[255];
        ++stats->binSplitCount[255];
        
        return (void*)((char*)allocated + HEADER_SIZE);
    
    }
    
    assert(false && "Unreachable");
    return nullptr;
}

// In FreeMem (replace existing function starting at 3102):
/// \brief Frees allocated memory and coalesces with adjacent free blocks.
/// 
/// Algorithm:
/// 1. Locate the block from the pointer; return if null.
/// 2. Perform left coalescing in a loop: while the previous block is free and leftMerges < sideCoalescing, unlink it from its bin, merge it into the current block by adjusting sizes and shifting the block pointer left, update merge stats.
/// 3. Perform right coalescing in a loop: while the next block is free or wild and rightMerges < sideCoalescing, unlink it, merge into current by adjusting sizes, update next-next prevSize; if it was wild, flag mergedToWild and break the loop.
/// 4. If mergedToWild, set the block state to WILD_BLOCK and update wild pointer; else, set to FREE and push to the appropriate bin.
/// 5. Update global free memory size, free count stats, and final next block's prevSize.
///
/// This handles chains of adjacent free blocks up to the limit per side.
/// 
/// \param ptr Pointer returned by TryMalloc (must not be nullptr).
/// \param sideCoalescing Maximum number of merges per side (0 = no coalescing, defaults to UINT_MAX for unlimited).
inline void FreeMem(priv::AllocTable* at, void* ptr, unsigned sideCoalescing = UINT_MAX) noexcept {
    using namespace priv;

    if (ptr == nullptr) return;

    FreeAllocHeader* block = (FreeAllocHeader*)((char*)ptr - HEADER_SIZE);
    assert(block->thisSize.state == (U32)AllocState::USED);

    AllocHeader* nextBlock = NextHeaderPtr((AllocHeader*)block);
    Size blockSize = block->thisSize.size;
    unsigned origBinIdx = GetFreeListBinIndex((AllocHeader*)block);  // For stats

    // Step 2: Left coalescing loop - merge previous free blocks backwards
    unsigned leftMerges = 0;
    while (leftMerges < sideCoalescing) {
        AllocHeader* prevBlock = PrevHeaderPtr((AllocHeader*)block);
        if (prevBlock->thisSize.state != (U32)AllocState::FREE) break;  // Stop if not free

        FreeAllocHeader* prevFree = (FreeAllocHeader*)prevBlock;
        int prevBin = GetFreeListBinIndex((AllocHeader*)prevFree);

        // Unlink the previous free block from its bin
        DetachLink(&prevFree->freeListLink);
        --at->freeListBinsCount[prevBin];
        if (at->freeListBinsCount[prevBin] == 0) ClearAllocFreeBinBit(at, prevBin);

        // Merge: extend prev into current by adding sizes, shift block to prev
        prevBlock->thisSize.size += blockSize;
        block = (FreeAllocHeader*)prevBlock;
        blockSize = block->thisSize.size;

        // Update stats for this merge
        AllocStats* stats = &at->stats;
        ++stats->binMergeCount[prevBin];

        ++leftMerges;
    }

    // Step 3: Right coalescing loop - merge next free/wild blocks forwards
    unsigned rightMerges = 0;
    bool mergedToWild = false;
    while (rightMerges < sideCoalescing) {
        nextBlock = NextHeaderPtr((AllocHeader*)block);  // Refresh next after any prior merges
        AllocState nextState = (AllocState)nextBlock->thisSize.state;
        if (nextState != AllocState::FREE && nextState != AllocState::WILD_BLOCK) break;  // Stop if not free/wild

        FreeAllocHeader* nextFree = (FreeAllocHeader*)nextBlock;
        Size nextSize = nextBlock->thisSize.size;

        if (nextState == AllocState::FREE) {
            int nextBin = GetFreeListBinIndex((AllocHeader*)nextFree);

            // Unlink the next free block from its bin
            DetachLink(&nextFree->freeListLink);
            --at->freeListBinsCount[nextBin];
            if (at->freeListBinsCount[nextBin] == 0) ClearAllocFreeBinBit(at, nextBin);

            // Update stats for this merge
            AllocStats* stats = &at->stats;
            ++stats->binMergeCount[nextBin];
        } else {  // Wild block
            assert(nextFree == at->wildBlock);

            // Unlink the wild block
            DetachLink(&nextFree->freeListLink);
            at->freeListBinsCount[255] = 0;
            ClearAllocFreeBinBit(at, 255);

            // Update stats and flag
            AllocStats* stats = &at->stats;
            ++stats->binMergeCount[255];
            mergedToWild = true;
        }

        // Merge: extend current into next by adding sizes
        block->thisSize.size += nextSize;
        blockSize = block->thisSize.size;

        // Update the block after next's prevSize to point to the expanded current
        AllocHeader* nextNext = NextHeaderPtr(nextBlock);
        nextNext->prevSize = block->thisSize;

        ++rightMerges;

        // If we merged the wild block, stop further right coalescing
        if (mergedToWild) break;
    }

    // Step 4: Set final state based on whether we merged to wild
    if (mergedToWild) {
        block->thisSize.state = (U32)AllocState::WILD_BLOCK;
        at->wildBlock = block;
        // Wild doesn't get pushed to a bin
    } else {
        block->thisSize.state = (U32)AllocState::FREE;
        int newBinIdx = (blockSize / ALIGNMENT) - 1;
        DLink* newStack = &at->freeListBins[newBinIdx];
        PushLink(newStack, &block->freeListLink);
        ++at->freeListBinsCount[newBinIdx];
        if (at->freeListBinsCount[newBinIdx] == 1) SetAllocFreeBinBit(at, newBinIdx);

        // Update pool stats for the new free block
        AllocStats* stats = &at->stats;
        ++stats->binPoolCount[newBinIdx];
    }

    // Step 5: Update global stats and final metadata
    at->freeMemSize += blockSize;  // Add the total merged size to free memory
    AllocStats* stats = &at->stats;
    ++stats->binFreeCount[origBinIdx];

    // Ensure the final next block's prevSize is updated
    nextBlock = NextHeaderPtr((AllocHeader*)block);
    nextBlock->prevSize = block->thisSize;
}


// Kernel API implementation
// ----------------------------------------------------------------------------------------------------------------
template <typename... Args>
inline int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept {
    using namespace priv;

    if (InitKernel(config) < 0) {
        return -1;
    }

    KernelTaskHdl hdl = KernelTaskProc(mainProc, std::forward<Args>(args) ...);
    gKernel.kernelTask = hdl;
    hdl.resume();

    FiniKernel();

    return 0;
}

} // namespace ak


