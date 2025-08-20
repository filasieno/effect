#pragma once

#include <coroutine>
#include <cassert>
#include <cstdlib>
#include <print> // IWYU pragma: keep
#include <immintrin.h>

#include <liburing.h>

#include "ak/defs.hpp"
#include "ak/utl.hpp" 
#include "ak/impl_utl.hpp" // IWYU pragma: keep

namespace ak 
{
    struct Event;
    struct ExitOp;

    /// \brief A handle to a cooperative thread (C++20 coroutine)
    /// \ingroup CThread
    struct CThread {
        struct Context;
        using Hdl = std::coroutine_handle<Context>;
        using promise_type = Context;

        /// \brief Idenfies the state of a task
        /// \ingroup Task
        enum class State
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
        
        struct Context {
            using DLink = utl::DLink;
            struct SizeProbe {};
    
            struct InitialSuspendTaskOp {
                constexpr Bool await_ready() const noexcept { return false; }
                Void           await_suspend(Hdl hdl) const noexcept;
                constexpr Void await_resume() const noexcept {}
            };
    
            struct FinalSuspendTaskOp {
                constexpr Bool await_ready() const noexcept { return false; }
                Hdl            await_suspend(Hdl hdl) const noexcept;
                constexpr Void await_resume() const noexcept {}
            };
    
            static Void*   operator new(std::size_t n) noexcept;
            static Void    operator delete(Void* ptr, std::size_t sz);
            static CThread get_return_object_on_allocation_failure() noexcept { return {}; }
    
            template <typename... Args>
            Context(Args&&...);
            ~Context();
            
            CThread        get_return_object() noexcept { return {Hdl::from_promise(*this)};}
            constexpr auto initial_suspend() const noexcept { return InitialSuspendTaskOp {}; }
            constexpr auto final_suspend () const noexcept { return FinalSuspendTaskOp{}; }
            Void           return_value(int value) noexcept;
            Void           unhandled_exception() noexcept  { std::abort(); /* unreachable */ }
    
            State state;
            int   res;
            U32   prepared_io;
            DLink wait_link;     //< Used to enqueue tasks waiting for Critical Section
            DLink tasklist_link; //< Global Task list
            DLink awaiter_list;  //< The list of all tasks waiting for this task
        };

        CThread() = default;
        CThread(const CThread&) = default;
        CThread(CThread&&) = default;
        CThread& operator=(const CThread&) = default;
        CThread& operator=(CThread&&) = default;
        ~CThread() = default;

        CThread(const Hdl& hdl) : hdl(hdl) {}
        CThread& operator=(const Hdl& hdl) {
            this->hdl = hdl;
            return *this;
        }    
        Bool operator==(const CThread& other) const noexcept { return hdl == other.hdl; }
        operator Bool() const noexcept { return hdl.address() != nullptr; }
        operator Hdl() const noexcept { return hdl; }
        
        Void reset() noexcept {
            hdl = Hdl{};
        }

        Hdl hdl;
    };
    const Char* to_string(CThread::State state) noexcept;
    
    inline CThread::Hdl to_handle(CThread::Context* cthread_context) noexcept {
        return CThread::Hdl::from_promise(*cthread_context);        
    }
    // Allocator
    // ----------------------------------------------------------------------------------------------------------------
    // alloc::BlockState
    // alloc::BlockKind
    // alloc::BlockDesc
    // alloc::FreeBlock
    // alloc::UsedBlock
    // alloc::Table
    // alloc::Stats

    enum class AllocBlockState {
        INVALID              = 0b0000,
        USED                 = 0b0010,
        FREE                 = 0b0001,
        WILD_BLOCK           = 0b0011,
        BEGIN_SENTINEL       = 0b0100,
        LARGE_BLOCK_SENTINEL = 0b0110,
        END_SENTINEL         = 0b1100,
    };

    const Char* to_string(AllocBlockState s) noexcept;

    enum class AllocKind {
        INVALID = 0,
        GENERIC_MALLOC,
        PROMISE,
        FREE_SEGMENT_INDEX_LEAF,
        FREE_SEGMENT_INDEX_INNER,
        FREE_SEGMENT_INDEX_LEAF_EXTENSION
    };
    struct AllocBlockDesc {
        U64 size  : 48;
        U64 state : 4;
        U64 kind  : 12;
    };

    struct AllocBlockHeader {
        AllocBlockDesc this_desc;
        AllocBlockDesc prev_desc;
    };

    struct AllocPooledFreeBlockHeader : public AllocBlockHeader {
        utl::DLink freelist_link;
    };
    static_assert(sizeof(AllocPooledFreeBlockHeader) == 32);

    /// \internal 
    /// \details The key of the AVL tree is `this_desc.size`.
    /// \ingroup Allocator
    struct AllocFreeBlockHeader : public AllocBlockHeader {         
        utl::DLink           multimap_link; // Multimap ring link
        AllocFreeBlockHeader* parent;   // AVL Parent node for intrusive "detach"     
        AllocFreeBlockHeader* left;     // AVL Left child
        AllocFreeBlockHeader* right;    // AVL Right child
        I32 height;                     // if height is < 0 the is not a tree node but is a list node in the tree
        I32 balance;                    // AVL Balance factor
        
    };
    static_assert(sizeof(AllocFreeBlockHeader) == 64, "AllocFreeBlockHeader size is not 64 bytes");

    struct AllocStats {
        // Freelist bins count (0..63). Keep this constant for freelist arrays
        static constexpr int ALLOCATOR_BIN_COUNT = 64;
        // Stats bins count: add index 64 for tree allocations and 65 for wild block
        static constexpr int STATS_BIN_COUNT = 66;
        // Special stats indices for clarity
        static constexpr int STATS_IDX_TREE = 64;
        static constexpr int STATS_IDX_WILD = 65;

        Size alloc_counter[STATS_BIN_COUNT];
        Size realloc_counter[STATS_BIN_COUNT];
        Size free_counter[STATS_BIN_COUNT];
        Size failed_counter[STATS_BIN_COUNT];
        Size split_counter[STATS_BIN_COUNT];
        Size merged_counter[STATS_BIN_COUNT];
        Size reused_counter[STATS_BIN_COUNT];
        Size pooled_counter[STATS_BIN_COUNT];
    };

    struct AllocTable {
        static constexpr int ALLOCATOR_BIN_COUNT = AllocStats::ALLOCATOR_BIN_COUNT;
        // FREE LIST MANAGEMENT
        alignas(8)  U64        freelist_mask;   // 64-bit mask
        alignas(64) utl::DLink freelist_head[ALLOCATOR_BIN_COUNT];
        alignas(64) U32        freelist_count[ALLOCATOR_BIN_COUNT];

        // HEAP BOUNDARY MANAGEMENT
        alignas(8) Char* heap_begin;
        alignas(8) Char* heap_end;
        alignas(8) Char* mem_begin;
        alignas(8) Char* mem_end;
        
        // MEMORY ACCOUNTING
        Size mem_size;
        Size free_mem_size;
        Size max_free_block_size;
        
        // ALLOCATION STATISTICS
        AllocStats stats;
        
        // SENTINEL BLOCKS
        alignas(8) AllocPooledFreeBlockHeader* sentinel_begin;
        alignas(8) AllocPooledFreeBlockHeader* sentinel_end;
        alignas(8) AllocPooledFreeBlockHeader* wild_block;
        alignas(8) AllocFreeBlockHeader*       root_free_block;
    };

    namespace priv {
        // TODO: move to alloc table
        constexpr U64 ALLOC_STATE_IS_USED_MASK     = 0;
        constexpr U64 ALLOC_STATE_IS_FREE_MASK     = 1;
        constexpr U64 ALLOC_STATE_IS_SENTINEL_MASK = 4;    

    } // namespace priv


    // Kernel
    // ----------------------------------------------------------------------------------------------------------------
    
    struct BootCThread {
        struct Context {
            using InitialSuspend = std::suspend_always;
            using FinalSuspend   = std::suspend_never;
    
            static Void*       operator new(std::size_t) noexcept;
            static Void        operator delete(Void*, std::size_t) noexcept {};
            static BootCThread get_return_object_on_allocation_failure() noexcept { std::abort(); /* unreachable */ }
    
            template <typename... Args>
            Context(CThread(*)(Args ...) noexcept, Args... ) noexcept : exit_code(0) {}
            
            constexpr BootCThread    get_return_object() noexcept { return {Hdl::from_promise(*this)}; }
            constexpr InitialSuspend initial_suspend() noexcept { return {}; }
            constexpr FinalSuspend   final_suspend() noexcept { return {}; }
            constexpr Void           return_void() noexcept { }
            constexpr Void           unhandled_exception() noexcept { std::abort(); } 
    
            int exit_code;
        };

        using promise_type = Context;

        using Hdl = std::coroutine_handle<Context>;

        BootCThread() = default;
        BootCThread(const Hdl& hdl) : hdl(hdl) {}
        BootCThread(const BootCThread& other) noexcept = default;
        BootCThread& operator=(const BootCThread& other) = default;
        BootCThread(BootCThread&& other) = default;
        ~BootCThread() = default;

        operator Hdl() const noexcept { return hdl; }

        Hdl hdl;
    };
    
    struct Kernel {
        using DLink = utl::DLink;
        
        // Allocation table
        AllocTable alloc_table;
        
        // Task management
        Char        boot_cthread_frame_buffer[64];
        BootCThread boot_cthread;
        CThread     current_cthread;
        CThread     scheduler_cthread;
        CThread     main_cthread;
        
        DLink   zombie_list;
        DLink   ready_list;
        DLink   cthread_list;
        Void*   mem;
        Size    mem_size; // remove mem begin+end
        I32     main_cthread_exit_code;
        // Count state variables
        I32     cthread_count;
        I32     ready_cthread_count;
        I32     waiting_cthread_count;
        I32     iowaiting_cthread_count;
        I32     zombie_cthread_count;
        I32     interrupted;
        
        // IOManagement
        io_uring io_uring_state;
        U32      ioentry_count;
    };

    // Global kernel instance declaration (defined in ak.hpp)
    alignas(64) inline Kernel global_kernel_state;
    
    // Main Routine
    struct KernelConfig {
        Void*    mem;
        Size     memSize;
        unsigned ioEntryCount;
    };


    int  init_kernel(KernelConfig* config) noexcept;
    Void fini_kernel() noexcept;
    template <typename... Args>
    int run_main(CThread (*co_main)(Args ...) noexcept, Args... args) noexcept;
    
    //
    // Declarations for ops 
    namespace op {
        struct ResumeCThread {
            using Hdl = CThread::Hdl;
            explicit ResumeCThread(CThread ct) : hdl(ct.hdl) {};
    
            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
    
            Hdl hdl;
        };

        struct JoinCThread {
            using Hdl = CThread::Hdl;
            explicit JoinCThread(Hdl hdl) : hdl(hdl) {};
    
            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr int  await_resume() const noexcept { return hdl.promise().res; }
    
            Hdl hdl;
        };

        struct Suspend {
            using Hdl = CThread::Hdl;

            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
        };

        struct GetCurrentTask {
            using Hdl = CThread::Hdl;
            constexpr Bool await_ready() const noexcept { return false; }
            constexpr Hdl  await_suspend(Hdl hdl) noexcept;
            constexpr Hdl  await_resume() const noexcept { return hdl; }

            Hdl hdl;
        };

        struct ExecIO {
            using Hdl = CThread::Hdl;
            constexpr Bool await_ready() const noexcept { return false; }
            constexpr Hdl  await_suspend(Hdl hdl) noexcept;
            constexpr int  await_resume() const noexcept { return global_kernel_state.current_cthread.hdl.promise().res; }
        };

        struct WaitEvent {
            explicit WaitEvent(Event* event) : evt(event) {}

            constexpr Bool         await_ready() const noexcept { return false; }
            constexpr CThread::Hdl await_suspend(CThread::Hdl hdl) const noexcept;
            constexpr Void         await_resume() const noexcept {}

            Event* evt;
        };
    }
    // Declarations for ops 

    // CThread routines
    Bool                is_valid(CThread thread) noexcept;
    Bool                is_done(CThread thread) noexcept;
    CThread::Context*   get_context() noexcept;
    CThread::Context*   get_context(CThread thread) noexcept;
    CThread::State      get_state(CThread thread) noexcept;
    op::JoinCThread     join(CThread thread) noexcept;
    op::JoinCThread     operator co_await(CThread thread) noexcept;
    op::ResumeCThread   resume(CThread thread) noexcept;
    
    constexpr op::Suspend suspend() noexcept;

    // Remove
    constexpr op::GetCurrentTask get_cthread_context_async() noexcept; //< Duplicated remove.
    

    // IO Routines
    op::ExecIO io_open(const Char* path, int flags, mode_t mode) noexcept;
    op::ExecIO io_open_at(int dfd, const Char* path, int flags, mode_t mode) noexcept;
    op::ExecIO io_open_at_direct(int dfd, const Char* path, int flags, mode_t mode, unsigned file_index) noexcept;
    op::ExecIO io_close(int fd) noexcept;
    op::ExecIO io_close_direct(unsigned file_index) noexcept;
    op::ExecIO io_read(int fd, Void* buf, unsigned nbytes, __u64 offset) noexcept;
    op::ExecIO io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept;
    op::ExecIO io_read_fixed(int fd, Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    op::ExecIO io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    op::ExecIO io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    op::ExecIO io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    op::ExecIO io_write(int fd, const Void* buf, unsigned nbytes, __u64 offset) noexcept;
    op::ExecIO io_write_fixed(int fd, const Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
    op::ExecIO io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
    op::ExecIO io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
    op::ExecIO io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
    op::ExecIO io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    op::ExecIO io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept;
    op::ExecIO io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    op::ExecIO io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
    op::ExecIO io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
    op::ExecIO io_send(int sockfd, const Void* buf, size_t len, int flags) noexcept;
    op::ExecIO io_send_zc(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags) noexcept;
    op::ExecIO io_send_zc_fixed(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept;
    op::ExecIO io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    op::ExecIO io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept;
    op::ExecIO io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept;
    op::ExecIO io_recv(int sockfd, Void* buf, size_t len, int flags) noexcept;
    op::ExecIO io_recv_multishot(int sockfd, Void* buf, size_t len, int flags) noexcept;
    op::ExecIO io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept;
    op::ExecIO io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept;
    op::ExecIO io_socket(int domain, int type, int protocol, unsigned int flags) noexcept;
    op::ExecIO io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept;
    op::ExecIO io_mkdir(const Char* path, mode_t mode) noexcept;
    op::ExecIO io_mkdir_at(int dfd, const Char* path, mode_t mode) noexcept;
    op::ExecIO io_symlink(const Char* target, const Char* linkpath) noexcept;
    op::ExecIO io_symlink_at(const Char* target, int newdirfd, const Char* linkpath) noexcept;
    op::ExecIO io_link(const Char* oldpath, const Char* newpath, int flags) noexcept;
    op::ExecIO io_link_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, int flags) noexcept;
    op::ExecIO io_unlink(const Char* path, int flags) noexcept;
    op::ExecIO io_unlink_at(int dfd, const Char* path, int flags) noexcept;
    op::ExecIO io_rename(const Char* oldpath, const Char* newpath) noexcept;
    op::ExecIO io_rename_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, unsigned int flags) noexcept;

    // Allocator
    Void* try_alloc_mem(Size sz) noexcept;
    Void  free_mem(Void* ptr, U32 side_coalesching = UINT_MAX) noexcept;
    I32   defragment_mem(U64 millis_time_budget = ~0ull) noexcept;

    // Concurrency Tools
    struct Event {  
        utl::DLink wait_list;
    };

    Void          init(Event* event);
    int           signal(Event* event);
    int           signal_n(Event* event, int n);
    int           signal_all(Event* event);
    op::WaitEvent wait(Event* event);

}
