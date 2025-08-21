#pragma once

#include <coroutine>
#include <liburing.h>

#include "ak/base/base.hpp"   // IWYU pragma: keep
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

namespace ak { 
   
 
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
    
    //struct Event { priv::DLink wait_list; };
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

    // IO API (moved from ak/io)
    namespace op {
        struct ExecIO {
            using Hdl = CThread::Hdl;
            constexpr Bool await_ready() const noexcept  { return false; }
            constexpr I32  await_resume() const noexcept { return global_kernel_state.current_cthread.hdl.promise().res; }
            Hdl await_suspend(Hdl hdl) noexcept;
        };
    }

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

}