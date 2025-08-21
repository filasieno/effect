#pragma once

#include <coroutine>
#include <liburing.h>
#include <source_location>
#include <string_view>
#include <cstdio>
#include <format>
#include <print>


#define AK_PACKED_ATTR __attribute__((packed))
#define AK_OFFSET(TYPE, MEMBER) ((::ak::Size)((U64)&(((TYPE*)0)->MEMBER)))
#define AK_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define AK_ASSERT(cond, ...)         ::ak::priv::ensure((cond), #cond, std::source_location::current(), ##__VA_ARGS__)
#define AK_ASSERT_AT(loc, cond, ...) ::ak::priv::ensure((cond), #cond, loc                            , ##__VA_ARGS__)

namespace ak { 
    using Void  = void;
    using Bool  = bool;
    using Char  = char;
    using WChar = wchar_t;

    using U64  = unsigned long long;  
    using U32  = unsigned long; 
    using U16  = unsigned short;
    using U8   = unsigned char;

    using I64  = signed long long;  
    using I32  = signed int; 
    using I16  = signed short;
    using I8   = signed char;
    
    using Size = unsigned long long; 
    using ISize = signed long long; 
    using PtrDiff = signed long long;

    using F32 = float;
    using F64 = double;    

    namespace priv {

        #ifdef NDEBUG
            constexpr Bool IS_DEBUG_MODE = false;
        #else
            constexpr Bool IS_DEBUG_MODE = true;   
        #endif
        constexpr Bool ENABLE_AVX2      = false;
        constexpr Bool TRACE_DEBUG_CODE = false;
        
        constexpr Bool ENABLE_FULL_INVARIANT_CHECKS = true;

        constexpr U64 CACHE_LINE = 64;

        struct DLink {
            DLink* next;
            DLink* prev;
        };

        template <typename... Args>
        inline Void ensure(Bool condition, const Char* expression_text, const std::source_location loc, const std::string_view fmt = {}, Args&&... args) noexcept;

    } // namespace ak::priv
 

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
            using DLink = priv::DLink;
            struct SizeProbe {};
    
            struct InitialSuspendTaskOp {
                constexpr Bool await_ready() const noexcept  { return false; }
                constexpr Void await_resume() const noexcept {}
                Void           await_suspend(Hdl hdl) const noexcept;
            };
    
            struct FinalSuspendTaskOp {
                constexpr Bool await_ready() const noexcept  { return false; }
                constexpr Void await_resume() const noexcept {}
                Hdl            await_suspend(Hdl hdl) const noexcept;
            };
    
            static Void*   operator new(std::size_t n) noexcept;
            static Void    operator delete(Void* ptr, std::size_t sz);
            static CThread get_return_object_on_allocation_failure() noexcept { return {}; }
    
            template <typename... Args>
            Context(Args&&...);
            ~Context();
            
            CThread        get_return_object() noexcept     { return { Hdl::from_promise(*this) }; }
            constexpr auto initial_suspend() const noexcept { return InitialSuspendTaskOp {}; }
            constexpr auto final_suspend () const noexcept  { return FinalSuspendTaskOp{}; }
            Void           return_value(int value) noexcept;
            Void           unhandled_exception() noexcept;

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
        priv::DLink freelist_link;
    };
    static_assert(sizeof(AllocPooledFreeBlockHeader) == 32);

    /// \internal 
    /// \details The key of the AVL tree is `this_desc.size`.
    /// \ingroup Allocator
    struct AllocFreeBlockHeader : public AllocBlockHeader {         
        priv::DLink            multimap_link; // Multimap ring link
        AllocFreeBlockHeader* parent;        // AVL Parent node for intrusive "detach"     
        AllocFreeBlockHeader* left;          // AVL Left child
        AllocFreeBlockHeader* right;         // AVL Right child
        I32                   height;        // if height is < 0 the is not a tree node but is a list node in the tree
        I32                   balance;       // AVL Balance factor
        
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
        alignas(64) priv::DLink freelist_head[ALLOCATOR_BIN_COUNT];
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
            static BootCThread
            get_return_object_on_allocation_failure() noexcept;

            template <typename... Args>
            Context(CThread(*)(Args ...) noexcept, Args... ) noexcept : exit_code(0) {}
            
            constexpr BootCThread    get_return_object() noexcept { return {Hdl::from_promise(*this)}; }
            constexpr InitialSuspend initial_suspend() noexcept { return {}; }
            constexpr FinalSuspend   final_suspend() noexcept { return {}; }
            constexpr Void           return_void() noexcept { }
            constexpr Void           unhandled_exception() noexcept;
    
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
        using DLink = priv::DLink;
        
        // Allocation table
        AllocTable alloc_table;
        
        // Task management
        Char        boot_cthread_frame_buffer[64];
        BootCThread boot_cthread;
        CThread     current_cthread;
        CThread     scheduler_cthread;
        CThread     main_cthread;
        
        DLink       zombie_list;
        DLink       ready_list;
        DLink       cthread_list;
        Void*       mem;
        Size        mem_size; // remove mem begin+end
        I32         main_cthread_exit_code;

        // Count state variables
        I32         cthread_count;
        I32         ready_cthread_count;
        I32         waiting_cthread_count;
        I32         iowaiting_cthread_count;
        I32         zombie_cthread_count;
        I32         interrupted;
        
        // IOManagement
        io_uring    io_uring_state;
        U32         ioentry_count;
    };

    extern Kernel global_kernel_state;
    
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
    
    struct Event;
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
    op::ExecIO io_open_at2(int dfd, const Char* path, struct open_how* how) noexcept;
    op::ExecIO io_open_at2_direct(int dfd, const Char* path, struct open_how* how, unsigned file_index) noexcept;
    op::ExecIO io_open_direct(const Char* path, int flags, mode_t mode, unsigned file_index) noexcept;
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
    #if defined(IORING_OP_BIND)
    op::ExecIO io_bind(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
    #endif
    #if defined(IORING_OP_LISTEN)
    op::ExecIO io_listen(int fd, int backlog) noexcept;
    #endif
    op::ExecIO io_send(int sockfd, const Void* buf, size_t len, int flags) noexcept;
    op::ExecIO io_send_bundle(int sockfd, size_t len, int flags) noexcept;
    op::ExecIO io_sendto(int sockfd, const Void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen) noexcept;
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
    #if defined(IORING_FILE_INDEX_ALLOC)
    op::ExecIO io_socket_direct_alloc(int domain, int type, int protocol, unsigned int flags) noexcept;
    #endif
    #if defined(IORING_OP_PIPE)
    op::ExecIO io_pipe(int* fds, unsigned int flags) noexcept;
    op::ExecIO io_pipe_direct(int* fds, unsigned int pipe_flags) noexcept;
    #endif
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
    op::ExecIO io_sync(int fd, unsigned fsync_flags) noexcept;
    op::ExecIO io_sync_file_range(int fd, unsigned len, __u64 offset, int flags) noexcept;
    op::ExecIO io_fallocate(int fd, int mode, __u64 offset, __u64 len) noexcept;
    op::ExecIO io_statx(int dfd, const Char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept;
    op::ExecIO io_fadvise(int fd, __u64 offset, __u32 len, int advice) noexcept;
    op::ExecIO io_fadvise64(int fd, __u64 offset, off_t len, int advice) noexcept;
    op::ExecIO io_madvise(Void* addr, __u32 length, int advice) noexcept;
    op::ExecIO io_madvise64(Void* addr, off_t length, int advice) noexcept;
    op::ExecIO io_get_xattr(const Char* name, Char* value, const Char* path, unsigned int len) noexcept;
    op::ExecIO io_set_xattr(const Char* name, const Char* value, const Char* path, int flags, unsigned int len) noexcept;
    op::ExecIO io_fget_xattr(int fd, const Char* name, Char* value, unsigned int len) noexcept;
    op::ExecIO io_fset_xattr(int fd, const Char* name, const Char* value, int flags, unsigned int len) noexcept;
    op::ExecIO io_provide_buffers(Void* addr, int len, int nr, int bgid, int bid) noexcept;
    op::ExecIO io_remove_buffers(int nr, int bgid) noexcept;
    op::ExecIO io_poll_add(int fd, unsigned poll_mask) noexcept;
    op::ExecIO io_poll_multishot(int fd, unsigned poll_mask) noexcept;
    op::ExecIO io_poll_remove(__u64 user_data) noexcept;
    op::ExecIO io_poll_update(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept;
    op::ExecIO io_epoll_ctl(int epfd, int fd, int op, struct epoll_event* ev) noexcept;
    op::ExecIO io_epoll_wait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept;
    op::ExecIO io_timeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept;
    op::ExecIO io_timeout_remove(__u64 user_data, unsigned flags) noexcept;
    op::ExecIO io_timeout_update(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept;
    op::ExecIO io_link_timeout(struct __kernel_timespec* ts, unsigned flags) noexcept;
    op::ExecIO io_msg_ring(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept;
    op::ExecIO io_msg_ring_cqe_flags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept;
    op::ExecIO io_msg_ring_fd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept;
    op::ExecIO io_msg_ring_fd_alloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept;
    op::ExecIO io_waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept;
    op::ExecIO io_futex_wake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
    op::ExecIO io_futex_wait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
    op::ExecIO io_futex_waitv(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept;
    op::ExecIO io_fixed_fd_install(int fd, unsigned int flags) noexcept;
    op::ExecIO io_files_update(int* fds, unsigned nr_fds, int offset) noexcept;
    op::ExecIO io_shutdown(int fd, int how) noexcept;
    op::ExecIO io_ftruncate(int fd, loff_t len) noexcept;
    op::ExecIO io_cmd_sock(int cmd_op, int fd, int level, int optname, Void* optval, int optlen) noexcept;
    op::ExecIO io_cmd_discard(int fd, uint64_t offset, uint64_t nbytes) noexcept;
    op::ExecIO io_nop(__u64 user_data) noexcept;
    op::ExecIO io_splice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
    op::ExecIO io_tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
    op::ExecIO io_cancel64(__u64 user_data, int flags) noexcept;
    op::ExecIO io_cancel(Void* user_data, int flags) noexcept;
    op::ExecIO io_cancel_fd(int fd, unsigned int flags) noexcept;

    // Allocator
    Void* try_alloc_mem(Size sz) noexcept;
    Void  free_mem(Void* ptr, U32 side_coalesching = UINT_MAX) noexcept;
    I32   defragment_mem(U64 millis_time_budget = ~0ull) noexcept;

    // Concurrency Tools
    struct Event {  
        priv::DLink wait_list;
    };

    Void          init(Event* event);
    int           signal(Event* event);
    int           signal_n(Event* event, int n);
    int           signal_all(Event* event);
    op::WaitEvent wait(Event* event);

}


// ================================================================================================================
// Inline Implementations
// ================================================================================================================

namespace ak { namespace priv {

    template <typename... Args>
    inline Void ensure(Bool condition,
                        const Char* expression_text,
                        const std::source_location loc,
                        const std::string_view fmt,
                        Args&&... args) noexcept {
        constexpr const Char* RESET  = "\033[0m";
        constexpr const Char* RED    = "\033[1;31m"; 
        if (AK_UNLIKELY(!condition)) {
            std::print("{}{}:{}: Assertion '{}' failed{}", RED, loc.file_name(), (int)loc.line(), expression_text, RESET);
            if (fmt.size() > 0 && !std::is_constant_evaluated()) {
                std::fputs("; ", stdout);
                if constexpr (sizeof...(Args) > 0) {
                    auto arg_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
                    std::apply([&](auto&... refs){
                        auto fmt_args = std::make_format_args(refs...);
                        std::vprint_nonunicode(stdout, fmt, fmt_args);
                    }, arg_tuple);
                } else {
                    std::fwrite(fmt.data(), 1, fmt.size(), stdout);
                }
            }
            std::fputc('\n', stdout);
            std::fflush(stdout);
            std::abort();
        }
    } // ensure

}}


namespace ak { namespace priv {

    inline Void init_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        
        link->next = link;
        link->prev = link;
    }

    inline Bool is_dlink_detached(const DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);

        return link->next == link && link->prev == link;
    }

    inline Void detach_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        AK_ASSERT(link->next != nullptr);
        AK_ASSERT(link->prev != nullptr);
        
        if (is_dlink_detached(link)) return;
        link->next->prev = link->prev;
        link->prev->next = link->next;
        link->next = link;
        link->prev = link;
    }

    inline Void clear_dlink(DLink* link) noexcept {
        AK_ASSERT(link != nullptr);
        
        link->next = nullptr;
        link->prev = nullptr;
    }

    inline Void enqueue_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline DLink* dequeue_dlink(DLink* queue) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);
        if (is_dlink_detached(queue)) return nullptr;
        DLink* target = queue->prev;
        detach_dlink(target);
        return target;
    }

    inline Void insert_prev_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);

        link->next = queue;
        link->prev = queue->prev;

        link->next->prev = link;
        link->prev->next = link;
    }

    inline Void insert_next_dlink(DLink* queue, DLink* link) noexcept {
        AK_ASSERT(queue != nullptr);
        AK_ASSERT(link != nullptr);
        AK_ASSERT(queue->next != nullptr);
        AK_ASSERT(queue->prev != nullptr);

        link->next = queue->next;
        link->prev = queue;

        link->next->prev = link;
        queue->next = link;
    }

    inline Void push_dlink(DLink* stack, DLink* link) noexcept {
        insert_next_dlink(stack, link);
    }

    inline DLink* pop_dlink(DLink* stack) noexcept {
        AK_ASSERT(stack != nullptr);
        AK_ASSERT(stack->next != nullptr);
        AK_ASSERT(stack->prev != nullptr);
        AK_ASSERT(!is_dlink_detached(stack));
        
        DLink* target = stack->next;
        detach_dlink(target);
        return target;
    }

}} // namespace ak::utl