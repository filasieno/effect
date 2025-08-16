#pragma once

#include <coroutine>
#include <cassert>
#include <immintrin.h>
#include <print>
#include "liburing.h"

#include "ak/defs.hpp"
#include "ak/utl.hpp" 
#include "ak/impl_utl.hpp" // IWYU pragma: keep

namespace ak 
{
    struct TaskContext;
    struct ResumeTaskOp;
    struct JoinTaskOp;
    struct SuspendOp;
    struct GetCurrentTaskOp;
    struct ExecIOOp;
    struct WaitEventOp;
    struct ExitOp;

    /// \brief Idenfies the state of a task
    /// \ingroup Task
    enum class TaskState
    {
        INVALID = 0, ///< Invalid OR uninitialized state
        CREATED,     ///< Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
        READY,       ///< Ready for execution
        RUNNING,     ///< Currently running
        IO_WAITING,  ///< Waiting for IO
        WAITING,     ///< Waiting for an event
        ZOMBIE,      ///< Already dead
        DELETING     ///< Currently being deleted
    };
    const Char* to_string(TaskState state) noexcept;

    /// \brief Coroutine handle for a Task
    /// \ingroup Task
    using TaskHdl = std::coroutine_handle<TaskContext>;

    struct DefineTask {
        using promise_type = TaskContext;

        DefineTask(const TaskHdl& hdl) : hdl(hdl) {}
        operator TaskHdl() const noexcept { return hdl; }

        TaskHdl hdl;
    };

    /// \brief Defines a Task function type-erased pointer (no std::function)
    /// \ingroup Task
    template <typename... Args>
    using TaskFn = DefineTask(*)(Args...);

    // TODO: Add size Probing for TaskContext
    struct TaskContextSizeProbe {};

    struct TaskContext {
        using DLink = utl::DLink;

        struct InitialSuspendTaskOp {
            constexpr Bool await_ready() const noexcept { return false; }
            Void           await_suspend(TaskHdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
        };

        struct FinalSuspendTaskOp {
            constexpr Bool await_ready() const noexcept { return false; }
            TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
        };

        static Void* operator new(std::size_t n) noexcept;
        static Void  operator delete(Void* ptr, std::size_t sz);
        static TaskHdl        get_return_object_on_allocation_failure() noexcept { return {}; }

        template <typename... Args>
        TaskContext(Args&&...);
        ~TaskContext();
        
        TaskHdl        get_return_object() noexcept { return TaskHdl::from_promise(*this);}
        constexpr auto initial_suspend() noexcept { return InitialSuspendTaskOp{}; }
        constexpr auto final_suspend() noexcept { return FinalSuspendTaskOp{}; }
        Void           return_value(int value) noexcept;
        Void           unhandled_exception() noexcept  { std::abort(); /* unreachable */ }

        TaskState state;
        int       ioResult;
        unsigned  enqueuedIO;
        DLink     waitLink;                // Used to enqueue tasks waiting for Critical Section
        DLink     taskListLink;            // Global Task list
        DLink     awaitingTerminationList; // The list of all tasks waiting for this task
    };

    // Allocator
    // ----------------------------------------------------------------------------------------------------------------

    enum class AllocState {
        INVALID              = 0b0000,
        USED                 = 0b0010,
        FREE                 = 0b0001,
        WILD_BLOCK           = 0b0011,
        BEGIN_SENTINEL       = 0b0100,
        LARGE_BLOCK_SENTINEL = 0b0110,
        END_SENTINEL         = 0b1100,
    };
    const Char* ToString(AllocState s) noexcept;

    struct AllocSizeRecord {
        U64 size      : 48;
        U64 state     : 4;
        U64 _reserved : 12;
    };

    struct AllocHeader {
        AllocSizeRecord thisSize;
        AllocSizeRecord prevSize;
    };

    struct FreeAllocHeader {
        AllocSizeRecord thisSize;
        AllocSizeRecord prevSize;
        utl::DLink freeListLink;
    };
    static_assert(sizeof(FreeAllocHeader) == 32);

    struct AllocStats {
        static constexpr int ALLOCATOR_BIN_COUNT = 256;

        Size binAllocCount[ALLOCATOR_BIN_COUNT];
        Size binReallocCount[ALLOCATOR_BIN_COUNT];
        Size binFreeCount[ALLOCATOR_BIN_COUNT];
        Size binFailedCount[ALLOCATOR_BIN_COUNT];
        Size binSplitCount[ALLOCATOR_BIN_COUNT];
        Size binMergeCount[ALLOCATOR_BIN_COUNT];
        Size binReuseCount[ALLOCATOR_BIN_COUNT];
        Size binPoolCount[ALLOCATOR_BIN_COUNT];
    };

    struct AllocTable {
        static constexpr int ALLOCATOR_BIN_COUNT = AllocStats::ALLOCATOR_BIN_COUNT;
        using DLink = utl::DLink;
        // FREE LIST MANAGEMENT
        alignas(64) __m256i freeListbinMask;                         
        alignas(64) DLink   freeListBins[ALLOCATOR_BIN_COUNT];
        alignas(64) U32     freeListBinsCount[ALLOCATOR_BIN_COUNT];

        // HEAP BOUNDARY MANAGEMENT
        alignas(8) Char* heapBegin;
        alignas(8) Char* heapEnd;
        alignas(8) Char* memBegin;
        alignas(8) Char* memEnd;
        
        // MEMORY ACCOUNTING
        Size memSize;
        Size freeMemSize;
        Size maxFreeBlockSize;
        
        // ALLOCATION STATISTICS
        AllocStats stats;
        
        // SENTINEL BLOCKS
        alignas(8) FreeAllocHeader* beginSentinel;
        alignas(8) FreeAllocHeader* wildBlock;
        alignas(8) FreeAllocHeader* largeBlockSentinel;
        alignas(8) FreeAllocHeader* endSentinel;
    };

    namespace priv {

        constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

    } // namespace priv


    // Kernel
    // ----------------------------------------------------------------------------------------------------------------
    struct KernelTaskPromise;
    using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

    struct KernelTaskPromise {
        using InitialSuspend = std::suspend_always;
        using FinalSuspend   = std::suspend_never;

        static Void*         operator new(std::size_t) noexcept;
        static Void          operator delete(Void*, std::size_t) noexcept {};
        static KernelTaskHdl get_return_object_on_allocation_failure() noexcept { std::abort(); /* unreachable */ }

        template <typename... Args>
        KernelTaskPromise(DefineTask(*)(Args ...) noexcept, Args... ) noexcept : returnValue(0) {}
        
        KernelTaskHdl            get_return_object() noexcept { return KernelTaskHdl::from_promise(*this); }
        constexpr InitialSuspend initial_suspend() noexcept { return {}; }
        constexpr FinalSuspend   final_suspend() noexcept { return {}; }
        constexpr Void           return_void() noexcept { }
        constexpr Void           unhandled_exception() noexcept { std::abort(); } 

        int returnValue;
    };
    
    struct Kernel {
        using DLink = utl::DLink;
        
        // Allocation table
        AllocTable alloc_table;
        
        // Task management
        TaskHdl currentTaskHdl;
        TaskHdl schedulerTaskHdl;
        TaskHdl mainTaskHdl;
        DLink   zombieList;
        DLink   readyList;
        DLink   taskList;
        Void*   mem;
        Size    memSize;
        I32     mainTaskReturnValue;
        I32     taskCount;
        I32     readyCount;
        I32     waitingCount;
        I32     ioWaitingCount;
        I32     zombieCount;
        I32     interrupted;
        
        // Kernnel Storage 
        KernelTaskHdl kernelTask;
        Char bootTaskFrame[64];

        // IOManagement
        io_uring ioRing;
        U32      ioEntryCount;
    };

    // Global kernel instance declaration (defined in ak.hpp)
    alignas(64) inline Kernel gKernel;
    
    // Main Routine
    struct KernelConfig {
        Void*    mem;
        Size     memSize;
        unsigned ioEntryCount;
    };

    template <typename... Args>
    int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept;

    // Task routines
    Void                       ClearTaskHdl(TaskHdl* hdl) noexcept;
    Bool                       IsTaskHdlValid(TaskHdl hdl) noexcept;
    TaskContext*               GetTaskContext(TaskHdl hdl) noexcept;
    TaskContext*               GetTaskContext() noexcept;
    constexpr GetCurrentTaskOp GetCurrentTask() noexcept;
    constexpr SuspendOp        SuspendTask() noexcept;
    JoinTaskOp                 JoinTask(TaskHdl hdl) noexcept;
    JoinTaskOp                 operator co_await(TaskHdl hdl) noexcept;
    TaskState                  GetTaskState(TaskHdl hdl) noexcept;
    Bool                       IsTaskDone(TaskHdl hdl) noexcept;
    ResumeTaskOp               ResumeTask(TaskHdl hdl) noexcept;
    constexpr ExitOp           ExitTask(int value = 0) noexcept;

    // IO Routines
    ExecIOOp io_open(const Char* path, int flags, mode_t mode) noexcept;
    ExecIOOp io_open_at(int dfd, const Char* path, int flags, mode_t mode) noexcept;
    ExecIOOp io_open_at_direct(int dfd, const Char* path, int flags, mode_t mode, unsigned file_index) noexcept;
    ExecIOOp io_close(int fd) noexcept;
    ExecIOOp io_close_direct(unsigned file_index) noexcept;
    ExecIOOp io_read(int fd, Void* buf, unsigned nbytes, __u64 offset) noexcept;
    ExecIOOp io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept;
    ExecIOOp io_read_fixed(int fd, Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    ExecIOOp io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    ExecIOOp io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    ExecIOOp io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    ExecIOOp io_write(int fd, const Void* buf, unsigned nbytes, __u64 offset) noexcept;
    ExecIOOp io_write_fixed(int fd, const Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    ExecIOOp io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    ExecIOOp io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    ExecIOOp io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    ExecIOOp io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept;
    ExecIOOp io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
    ExecIOOp io_send(int sockfd, const Void* buf, size_t len, int flags) noexcept;
    ExecIOOp io_send_zc(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags) noexcept;
    ExecIOOp io_send_zc_fixed(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept;
    ExecIOOp io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept;
    ExecIOOp io_recv(int sockfd, Void* buf, size_t len, int flags) noexcept;
    ExecIOOp io_recv_multishot(int sockfd, Void* buf, size_t len, int flags) noexcept;
    ExecIOOp io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp io_socket(int domain, int type, int protocol, unsigned int flags) noexcept;
    ExecIOOp io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept;
    ExecIOOp io_mkdir(const Char* path, mode_t mode) noexcept;
    ExecIOOp io_mkdir_at(int dfd, const Char* path, mode_t mode) noexcept;
    ExecIOOp io_symlink(const Char* target, const Char* linkpath) noexcept;
    ExecIOOp io_symlink_at(const Char* target, int newdirfd, const Char* linkpath) noexcept;
    ExecIOOp io_link(const Char* oldpath, const Char* newpath, int flags) noexcept;
    ExecIOOp io_link_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, int flags) noexcept;
    ExecIOOp io_unlink(const Char* path, int flags) noexcept;
    ExecIOOp io_unlink_at(int dfd, const Char* path, int flags) noexcept;
    ExecIOOp io_rename(const Char* oldpath, const Char* newpath) noexcept;
    ExecIOOp io_rename_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, unsigned int flags) noexcept;

    // Allocator
    Void* try_alloc_mem(Size sz) noexcept;
    Void  free_mem(Void* ptr, U32 side_coalesching = UINT_MAX) noexcept;

    // Concurrency Tools
    struct Event {  
        utl::DLink waitingList;
    };

    Void InitEvent(Event* event);
    int  SignalEvent(Event* event);
    int  SignalEventSome(Event* event, int n);
    int  SignalEventAll(Event* event);
    WaitEventOp WaitEvent(Event* event);

}

namespace ak 
{
    // Declarations for ops 
    struct ResumeTaskOp {
        explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};

        constexpr Bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr Void await_resume() const noexcept {}

        TaskHdl hdl;
    };

    struct JoinTaskOp {
        explicit JoinTaskOp(TaskHdl hdl) : joinedTaskHdl(hdl) {};

        constexpr Bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr int  await_resume() const noexcept { return joinedTaskHdl.promise().ioResult; }

        TaskHdl joinedTaskHdl;
    };

    struct SuspendOp {
        constexpr Bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr Void await_resume() const noexcept {}
    };

    struct GetCurrentTaskOp {
        constexpr Bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept;
        constexpr TaskHdl await_resume() const noexcept { return hdl; }

        TaskHdl hdl;
    };

   

    struct ExecIOOp {
        constexpr Bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept;
        constexpr int     await_resume() const noexcept { return gKernel.currentTaskHdl.promise().ioResult; }
    };

    struct ExitOp {
        explicit ExitOp(int value = 0) : returnValue(value) {}
        ExitOp(ExitOp&&) = default;
        ExitOp(const ExitOp&) = delete;
        ExitOp& operator=(const ExitOp&) = delete;
        ExitOp& operator=(ExitOp&&) = default;
        ~ExitOp() = default;

        constexpr Bool    await_ready() const noexcept { return false; }
        constexpr int     await_resume() const noexcept { return returnValue; }
        
        TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept { 
            (Void)currentTaskHdl;
            std::print("unimplemented ExitOp\n");
            std::fflush(stdout);
            std::abort(); 
        }

        int returnValue;
    };

    struct WaitEventOp {
        WaitEventOp(Event* event) : evt(event) {}

        constexpr Bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept;
        constexpr Void    await_resume() const noexcept {}

        Event* evt;
    };
}
