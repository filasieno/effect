#pragma once

#include <coroutine>
#include <cassert>
#include <immintrin.h>
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
    struct WaitOp;

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
    const char* ToString(TaskState state) noexcept;

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

    struct TaskContext {
        using DLink = utl::DLink;

        struct InitialSuspendTaskOp {
            constexpr bool await_ready() const noexcept { return false; }
            void           await_suspend(TaskHdl hdl) const noexcept;
            constexpr void await_resume() const noexcept {}
        };

        struct FinalSuspendTaskOp {
            constexpr bool await_ready() const noexcept { return false; }
            TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
            constexpr void await_resume() const noexcept {}
        };

        void* operator new(std::size_t n) noexcept;
        void  operator delete(void* ptr, std::size_t sz);

        template <typename... Args>
        TaskContext(Args&&...);

        ~TaskContext();

        TaskHdl        get_return_object() noexcept    { return TaskHdl::from_promise(*this);}
        constexpr auto initial_suspend() noexcept      { return InitialSuspendTaskOp{}; }
        constexpr auto final_suspend() noexcept        { return FinalSuspendTaskOp{}; }
        void           return_void() noexcept;
        void           unhandled_exception() noexcept  { assert(false); }

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
    const char* ToString(AllocState s) noexcept;

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
        Size binSplitCount[ALLOCATOR_BIN_COUNT];
        Size binMergeCount[ALLOCATOR_BIN_COUNT];
        Size binReuseCount[ALLOCATOR_BIN_COUNT];
        Size binPoolCount[ALLOCATOR_BIN_COUNT];
    };

    namespace priv {

        struct KernelTaskPromise;
        using KernelTaskHdl = std::coroutine_handle<KernelTaskPromise>;

        struct AllocTable {
            static constexpr int ALLOCATOR_BIN_COUNT = AllocStats::ALLOCATOR_BIN_COUNT;
            // FREE LIST MANAGEMENT
            alignas(64) __m256i    freeListbinMask;                         
            alignas(64) utl::DLink freeListBins[ALLOCATOR_BIN_COUNT];
            alignas(64) U32        freeListBinsCount[ALLOCATOR_BIN_COUNT];

            // HEAP BOUNDARY MANAGEMENT
            alignas(8) char* heapBegin;
            alignas(8) char* heapEnd;
            alignas(8) char* memBegin;
            alignas(8) char* memEnd;
            
            // MEMORY ACCOUNTING
            Size memSize;
            Size usedMemSize;
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

        constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

    } // namespace priv


    // Kernel
    // ----------------------------------------------------------------------------------------------------------------

    struct Kernel {
        // Allocation table
        alignas(64) priv::AllocTable allocTable;
        
        // Task management
        TaskHdl             currentTaskHdl;
        TaskHdl             schedulerTaskHdl;
        utl::DLink          zombieList;
        utl::DLink          readyList;
        utl::DLink          taskList;
        void*               mem;
        Size                memSize;
        priv::KernelTaskHdl kernelTask;
        int                 taskCount;
        int                 readyCount;
        int                 waitingCount;
        int                 ioWaitingCount;
        int                 zombieCount;
        int                 interrupted;
        
        // Kernnel Storage 
        char                bootTaskFrame[48];

        // IOManagement
        io_uring ioRing;
        unsigned ioEntryCount;
    };

    // Global kernel instance declaration (defined in ak.hpp)
    alignas(64) inline Kernel gKernel;
    
    // Main Routine
    struct KernelConfig {
        void*    mem;
        Size     memSize;
        unsigned ioEntryCount;
    };

    template <typename... Args>
    int RunMain(KernelConfig* config, DefineTask(*mainProc)(Args ...) noexcept , Args... args) noexcept;

    // Task routines
    void                       ClearTaskHdl(TaskHdl* hdl) noexcept;
    bool                       IsTaskHdlValid(TaskHdl hdl) noexcept;
    TaskContext*               GetTaskContext(TaskHdl hdl) noexcept;
    TaskContext*               GetTaskContext() noexcept;
    constexpr GetCurrentTaskOp GetCurrentTask() noexcept;
    constexpr SuspendOp        SuspendTask() noexcept;
    JoinTaskOp                 JoinTask(TaskHdl hdl) noexcept;
    JoinTaskOp                 operator co_await(TaskHdl hdl) noexcept;
    TaskState                  GetTaskState(TaskHdl hdl) noexcept;
    bool                       IsTaskDone(TaskHdl hdl) noexcept;
    ResumeTaskOp               ResumeTask(TaskHdl hdl) noexcept;

    // IO Routines
    ExecIOOp IOOpen(const char* path, int flags, mode_t mode) noexcept;
    ExecIOOp IOOpenAt(int dfd, const char* path, int flags, mode_t mode) noexcept;
    ExecIOOp IOOpenAtDirect(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept;
    ExecIOOp IOClose(int fd) noexcept;
    ExecIOOp IOCloseDirect(unsigned file_index) noexcept;
    ExecIOOp IORead(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept;
    ExecIOOp IOReadMultishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept;
    ExecIOOp IOReadFixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    ExecIOOp IOReadV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    ExecIOOp IOReadV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    ExecIOOp IOReadVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    ExecIOOp IOWrite(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept;
    ExecIOOp IOWriteFixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    ExecIOOp IOWriteV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    ExecIOOp IOWriteV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    ExecIOOp IOWriteVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    ExecIOOp IOAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp IOAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept;
    ExecIOOp IOMultishotAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp IOMultishotAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    ExecIOOp IOConnect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
    ExecIOOp IOSend(int sockfd, const void* buf, size_t len, int flags) noexcept;
    ExecIOOp IOSendZC(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept;
    ExecIOOp IOSendZCFixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept;
    ExecIOOp IOSendMsg(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp IOSendMsgZC(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp IOSendMsgZCFixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept;
    ExecIOOp IORecv(int sockfd, void* buf, size_t len, int flags) noexcept;
    ExecIOOp IORecvMultishot(int sockfd, void* buf, size_t len, int flags) noexcept;
    ExecIOOp IORecvMsg(int fd, struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp IORecvMsgMultishot(int fd, struct msghdr* msg, unsigned flags) noexcept;
    ExecIOOp IOSocket(int domain, int type, int protocol, unsigned int flags) noexcept;
    ExecIOOp IOSocketDirect(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept;
    ExecIOOp IOMkdir(const char* path, mode_t mode) noexcept;
    ExecIOOp IOMkdirAt(int dfd, const char* path, mode_t mode) noexcept;
    ExecIOOp IOSymlink(const char* target, const char* linkpath) noexcept;
    ExecIOOp IOSymlinkAt(const char* target, int newdirfd, const char* linkpath) noexcept;
    ExecIOOp IOLink(const char* oldpath, const char* newpath, int flags) noexcept;
    ExecIOOp IOLinkAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept;
    ExecIOOp IOUnlink(const char* path, int flags) noexcept;
    ExecIOOp IOUnlinkAt(int dfd, const char* path, int flags) noexcept;
    ExecIOOp IORename(const char* oldpath, const char* newpath) noexcept;
    ExecIOOp IORenameAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept;

    // Concurrency Tools
    struct Event {  
        utl::DLink waitingList;
    };

    void   InitEvent(Event* event);
    int    SignalOne(Event* event);
    int    SignalSome(Event* event, int n);
    int    SignalAll(Event* event);
    WaitOp WaitEvent(Event* event);

}

namespace ak 
{
    // Declarations for ops 
    struct ResumeTaskOp {
        explicit ResumeTaskOp(TaskHdl hdl) : hdl(hdl) {};

        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl hdl;
    };

    struct JoinTaskOp {
        explicit JoinTaskOp(TaskHdl hdl) : joinedTaskHdl(hdl) {};

        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl currentTaskHdl) const noexcept;
        constexpr void await_resume() const noexcept {}

        TaskHdl joinedTaskHdl;
    };

    struct SuspendOp {
        constexpr bool await_ready() const noexcept { return false; }
        TaskHdl        await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {}
    };

    struct GetCurrentTaskOp {
        constexpr bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) noexcept;
        constexpr TaskHdl await_resume() const noexcept { return hdl; }

        TaskHdl hdl;
    };

    struct WaitOp {
        WaitOp(Event* event) : evt(event) {}

        constexpr bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept;
        constexpr void    await_resume() const noexcept {}

        Event* evt;
    };

    struct ExecIOOp {
        constexpr bool    await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept;
        constexpr int     await_resume() const noexcept { return gKernel.currentTaskHdl.promise().ioResult; }
    };

}
