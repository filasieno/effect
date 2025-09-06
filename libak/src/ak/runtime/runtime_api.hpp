#pragma once

#include <coroutine>
#include <liburing.h>

#include "ak/base/base.hpp"   // IWYU pragma: keep
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

struct AkPromise;
struct AkTask;
using AkCoroutineHandle = std::coroutine_handle<AkPromise>;

/// \brief Idenfies the state of a task
/// \ingroup Task
enum class AkCoroutineState
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
const char* ak_to_string(AkCoroutineState state) noexcept;

struct AkPromise {

    struct InitialSuspend {
        constexpr bool await_ready() const noexcept  { return false; }
        constexpr void await_resume() const noexcept { }
        void           await_suspend(AkCoroutineHandle hdl) const noexcept;
    };

    struct FinalSuspend {
        constexpr bool  await_ready() const noexcept  { return false; }
        constexpr void  await_resume() const noexcept { }
        AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;
    };

    static void* operator new(std::size_t n) noexcept;
    static void  operator delete(void* ptr, std::size_t sz);
    static AkTask  get_return_object_on_allocation_failure() noexcept;

    AkPromise();
    ~AkPromise();
    
    AkTask         get_return_object() noexcept;
    constexpr auto initial_suspend() const noexcept { return InitialSuspend {}; }
    constexpr auto final_suspend () const noexcept { return FinalSuspend{}; }
    void         return_value(int value) noexcept;
    void         unhandled_exception() noexcept;

    AkCoroutineState state;
    int            res;
    AkU32            prepared_io;
    ak_dlink          wait_link;     //< Used to enqueue tasks waiting for Critical Section
    ak_dlink          tasklist_link; //< Global Task list
    ak_dlink          awaiter_list;  //< The list of all tasks waiting for this task
};

/// \brief A handle to a cooperative thread (C++20 coroutine)
/// \ingroup CThread
struct AkTask {
    using promise_type = AkPromise;

    AkTask() = default;
    AkTask(const AkTask&) = default;
    AkTask(AkTask&&) = default;
    AkTask& operator=(const AkTask&) = default;
    AkTask& operator=(AkTask&&) = default;
    ~AkTask() = default;

    AkTask(const AkCoroutineHandle& hdl) : hdl(hdl) {}
    AkTask& operator=(const AkCoroutineHandle& hdl) {
        this->hdl = hdl;
        return *this;
    }    
    bool operator==(const AkTask& other) const noexcept { return hdl == other.hdl; }
    operator bool() const noexcept { return hdl.address() != nullptr; }
    operator AkCoroutineHandle() const noexcept { return hdl; }
    
    void reset() noexcept {
        hdl = AkCoroutineHandle{};
    }

    AkCoroutineHandle hdl;
};

namespace ak { 
 
    // TODO: Remove
    struct BootCThread {
        struct Context {
            using InitialSuspend = std::suspend_always;
            using FinalSuspend   = std::suspend_never;
    
            static void*       operator new(std::size_t) noexcept;
            static void        operator delete(void*, std::size_t) noexcept {};
            static BootCThread
            get_return_object_on_allocation_failure() noexcept;

            template <typename... Args>
            Context(AkTask(*)(Args ...) noexcept, Args... ) noexcept : exit_code(0) {}
            
            constexpr BootCThread    get_return_object() noexcept { return {Hdl::from_promise(*this)}; }
            constexpr InitialSuspend initial_suspend() noexcept { return {}; }
            constexpr FinalSuspend   final_suspend() noexcept { return {}; }
            constexpr void         return_void() noexcept { }
            constexpr void         unhandled_exception() noexcept;
    
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
}

struct AkKernel {        
    // Allocation table
    ak_alloc_table    alloc_table;
    
    // Task management
    char          boot_task_frame_buffer[64];
    ak::BootCThread boot_task;
    AkTask          current_task;
    AkTask          scheduler_task;
    AkTask          main_task;
    
    ak_dlink         zombie_list;
    ak_dlink         ready_list;
    ak_dlink         task_list;
    void*         mem_buffer;
    AkSize          mem_buffer_size; // remove mem begin+end
    int           main_task_exit_code;

    // Count state variables
    int           task_count;
    int           ready_task_count;
    int           waiting_task_count;
    int           iowaiting_task_count;
    int           zombie_task_count;
    int           interrupted;
    
    // IOManagement
    io_uring        io_uring_state;
    AkU32           io_uring_entry_count;
};
extern AkKernel global_kernel_state;

struct AkKernelConfig {
    void*    mem_buffer;
    AkSize     mem_buffer_size;
    unsigned   io_uring_entry_count;
};

struct AkResumeTaskOp {
    explicit AkResumeTaskOp(AkTask task) : hdl(task.hdl) {};

    constexpr bool  await_ready() const noexcept  { return false; }
    constexpr void  await_resume() const noexcept { }

    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;

    AkCoroutineHandle hdl;
};

struct AkJoinTaskOp {
    
    explicit AkJoinTaskOp(AkCoroutineHandle hdl) : hdl(hdl) {};

    constexpr bool  await_ready() const noexcept  { return false; }
    constexpr int     await_resume() const noexcept { return hdl.promise().res; }

    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;

    AkCoroutineHandle hdl;
};

struct AkSuspendTaskOp {
    constexpr bool  await_ready() const noexcept  { return false; }
    constexpr void  await_resume() const noexcept { }

    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) const noexcept;
};

struct AkGetCurrentTaskOp {
    constexpr bool            await_ready() const noexcept  { return false; }
    constexpr AkCoroutineHandle await_resume() const noexcept { return hdl; }

    constexpr AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) noexcept;

    AkCoroutineHandle hdl;
};

struct AkIOOp {
    constexpr bool  await_ready()  const noexcept { return false; }
    constexpr int   await_resume() const noexcept { return global_kernel_state.current_task.hdl.promise().res; }

    AkCoroutineHandle await_suspend(AkCoroutineHandle hdl) noexcept;
};

template <typename... Args>
int ak_run_main(AkTask (*co_main)(Args ...) noexcept, Args... args) noexcept;

int                     ak_init_kernel(AkKernelConfig* config) noexcept;
void                    ak_fini_kernel() noexcept;
constexpr AkSuspendTaskOp ak_suspend_task() noexcept;
AkResumeTaskOp            ak_resume_task(AkTask task) noexcept;
AkCoroutineState          ak_get_task_state(AkTask task) noexcept;
bool                    ak_is_task_valid(AkTask task) noexcept;
bool                    ak_is_task_done(AkTask task) noexcept;
AkPromise*                ak_get_promise() noexcept;
AkPromise*                ak_get_promise(AkTask task) noexcept;

AkJoinTaskOp              operator co_await(AkTask task) noexcept;
AkJoinTaskOp              ak_join_task(AkTask task) noexcept;

AkGetCurrentTaskOp        ak_get_task_promise_async() noexcept; //< Remove Task

void*                   ak_alloc_mem(AkSize sz) noexcept;
void                    ak_free_mem(void*ptr, AkU32 side_coalesching = (AkU32)~0) noexcept;
int                     ak_defragment_mem(AkU64 millis_time_budget = ~0ull) noexcept;

// IO Routines
AkIOOp ak_os_io_open(const char* path, int flags, mode_t mode) noexcept;
AkIOOp ak_os_io_open_at(int dfd, const char* path, int flags, mode_t mode) noexcept;
AkIOOp ak_os_io_open_at_direct(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept;
AkIOOp ak_os_io_open_at2(int dfd, const char* path, struct open_how* how) noexcept;
AkIOOp ak_os_io_open_at2_direct(int dfd, const char* path, struct open_how* how, unsigned file_index) noexcept;
AkIOOp ak_os_io_open_direct(const char* path, int flags, mode_t mode, unsigned file_index) noexcept;
AkIOOp ak_os_io_close(int fd) noexcept;
AkIOOp ak_os_io_close_direct(unsigned file_index) noexcept;
AkIOOp ak_os_io_read(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept;
AkIOOp ak_os_io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept;
AkIOOp ak_os_io_read_fixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
AkIOOp ak_os_io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
AkIOOp ak_os_io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
AkIOOp ak_os_io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
AkIOOp ak_os_io_write(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept;
AkIOOp ak_os_io_write_fixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
AkIOOp ak_os_io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
AkIOOp ak_os_io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
AkIOOp ak_os_io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
AkIOOp ak_os_io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
AkIOOp ak_os_io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept;
AkIOOp ak_os_io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
AkIOOp ak_os_io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
AkIOOp ak_os_io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
#if defined(IORING_OP_BIND)
AkIOOp ak_os_io_bind(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
#endif
#if defined(IORING_OP_LISTEN)
AkIOOp ak_os_io_listen(int fd, int backlog) noexcept;
#endif
AkIOOp ak_os_io_send(int sockfd, const void* buf, size_t len, int flags) noexcept;
AkIOOp ak_os_io_send_bundle(int sockfd, size_t len, int flags) noexcept;
AkIOOp ak_os_io_sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen) noexcept;
AkIOOp ak_os_io_send_zc(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept;
AkIOOp ak_os_io_send_zc_fixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept;
AkIOOp ak_os_io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept;
AkIOOp ak_os_io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept;
AkIOOp ak_os_io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept;
AkIOOp ak_os_io_recv(int sockfd, void* buf, size_t len, int flags) noexcept;
AkIOOp ak_os_io_recv_multishot(int sockfd, void* buf, size_t len, int flags) noexcept;
AkIOOp ak_os_io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept;
AkIOOp ak_os_io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept;
AkIOOp ak_os_io_socket(int domain, int type, int protocol, unsigned int flags) noexcept;
AkIOOp ak_os_io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept;
#if defined(IORING_FILE_INDEX_ALLOC)
AkIOOp ak_os_io_socket_direct_alloc(int domain, int type, int protocol, unsigned int flags) noexcept;
#endif
#if defined(IORING_OP_PIPE)
AkIOOp ak_os_io_pipe(int* fds, unsigned int flags) noexcept;
AkIOOp ak_os_io_pipe_direct(int* fds, unsigned int pipe_flags) noexcept;
#endif
AkIOOp ak_os_io_mkdir(const char* path, mode_t mode) noexcept;
AkIOOp ak_os_io_mkdir_at(int dfd, const char* path, mode_t mode) noexcept;
AkIOOp ak_os_io_symlink(const char* target, const char* linkpath) noexcept;
AkIOOp ak_os_io_symlink_at(const char* target, int newdirfd, const char* linkpath) noexcept;
AkIOOp ak_os_io_link(const char* oldpath, const char* newpath, int flags) noexcept;
AkIOOp ak_os_io_link_at(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept;
AkIOOp ak_os_io_unlink(const char* path, int flags) noexcept;
AkIOOp ak_os_io_unlink_at(int dfd, const char* path, int flags) noexcept;
AkIOOp ak_os_io_rename(const char* oldpath, const char* newpath) noexcept;
AkIOOp ak_os_io_rename_at(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept;
AkIOOp ak_os_io_sync(int fd, unsigned fsync_flags) noexcept;
AkIOOp ak_os_io_sync_file_range(int fd, unsigned len, __u64 offset, int flags) noexcept;
AkIOOp ak_os_io_fallocate(int fd, int mode, __u64 offset, __u64 len) noexcept;
AkIOOp ak_os_io_statx(int dfd, const char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept;
AkIOOp ak_os_io_fadvise(int fd, __u64 offset, __u32 len, int advice) noexcept;
AkIOOp ak_os_io_fadvise64(int fd, __u64 offset, off_t len, int advice) noexcept;
AkIOOp ak_os_io_madvise(void* addr, __u32 length, int advice) noexcept;
AkIOOp ak_os_io_madvise64(void* addr, off_t length, int advice) noexcept;
AkIOOp ak_os_io_get_xattr(const char* name, char* value, const char* path, unsigned int len) noexcept;
AkIOOp ak_os_io_set_xattr(const char* name, const char* value, const char* path, int flags, unsigned int len) noexcept;
AkIOOp ak_os_io_fget_xattr(int fd, const char* name, char* value, unsigned int len) noexcept;
AkIOOp ak_os_io_fset_xattr(int fd, const char* name, const char* value, int flags, unsigned int len) noexcept;
AkIOOp ak_os_io_provide_buffers(void* addr, int len, int nr, int bgid, int bid) noexcept;
AkIOOp ak_os_io_remove_buffers(int nr, int bgid) noexcept;
AkIOOp ak_os_io_poll_add(int fd, unsigned poll_mask) noexcept;
AkIOOp ak_os_io_poll_multishot(int fd, unsigned poll_mask) noexcept;
AkIOOp ak_os_io_poll_remove(__u64 user_data) noexcept;
AkIOOp ak_os_io_poll_update(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept;
AkIOOp ak_os_io_epoll_ctl(int epfd, int fd, int op, struct epoll_event* ev) noexcept;
AkIOOp ak_os_io_epoll_wait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept;
AkIOOp ak_os_io_timeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept;
AkIOOp ak_os_io_timeout_remove(__u64 user_data, unsigned flags) noexcept;
AkIOOp ak_os_io_timeout_update(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept;
AkIOOp ak_os_io_link_timeout(struct __kernel_timespec* ts, unsigned flags) noexcept;
AkIOOp ak_os_io_msg_ring(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept;
AkIOOp ak_os_io_msg_ring_cqe_flags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept;
AkIOOp ak_os_io_msg_ring_fd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept;
AkIOOp ak_os_io_msg_ring_fd_alloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept;
AkIOOp ak_os_io_waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept;
AkIOOp ak_os_io_futex_wake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
AkIOOp ak_os_io_futex_wait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
AkIOOp ak_os_io_futex_waitv(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept;
AkIOOp ak_os_io_fixed_fd_install(int fd, unsigned int flags) noexcept;
AkIOOp ak_os_io_files_update(int* fds, unsigned nr_fds, int offset) noexcept;
AkIOOp ak_os_io_shutdown(int fd, int how) noexcept;
AkIOOp ak_os_io_ftruncate(int fd, loff_t len) noexcept;
AkIOOp ak_os_io_cmd_sock(int cmd_op, int fd, int level, int optname, void* optval, int optlen) noexcept;
AkIOOp ak_os_io_cmd_discard(int fd, uint64_t offset, uint64_t nbytes) noexcept;
AkIOOp ak_os_io_nop(__u64 user_data) noexcept;
AkIOOp ak_os_io_splice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
AkIOOp ak_os_io_tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
AkIOOp ak_os_io_cancel64(__u64 user_data, int flags) noexcept;
AkIOOp ak_os_io_cancel(void* user_data, int flags) noexcept;
AkIOOp ak_os_io_cancel_fd(int fd, unsigned int flags) noexcept;
