#include "ak/runtime/runtime.hpp" // IWYU pragma: keep

AkCoroutineHandle AkIOOp::await_suspend(AkCoroutineHandle current_context_hdl) noexcept {

    // Move current task to IO_WAITING and resume scheduler
    auto* current_context = ak_get_promise(current_context_hdl);
    AK_ASSERT(current_context->state == AkCoroutineState::RUNNING);
    current_context->state = AkCoroutineState::IO_WAITING;
    ++global_kernel_state.iowaiting_task_count;
    global_kernel_state.current_task.reset();
    runtime_check_invariants();
    runtime_dump_task_count();

    auto* sched_ctx = ak_get_promise(global_kernel_state.scheduler_task);
    AK_ASSERT(sched_ctx->state == AkCoroutineState::READY);
    sched_ctx->state = AkCoroutineState::RUNNING;
    ak_dlink_detach(&sched_ctx->wait_link);
    --global_kernel_state.ready_task_count;
    global_kernel_state.current_task = global_kernel_state.scheduler_task;
    runtime_check_invariants();
    runtime_dump_task_count();

    return global_kernel_state.scheduler_task;
}

template <typename PrepFn>
inline AkIOOp prepare_io_uring_op(PrepFn prep_fn) noexcept {
    AkPromise* ctx = ak_get_promise(global_kernel_state.current_task);
    unsigned int free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
    while (free_slots < 1) {
        int ret = io_uring_submit(&global_kernel_state.io_uring_state);
        if (ret < 0) { abort(); }
        free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
    }
    io_uring_sqe* sqe = io_uring_get_sqe(&global_kernel_state.io_uring_state);
    io_uring_sqe_set_data(sqe, (void*) ctx);
    prep_fn(sqe);
    ctx->res = 0;
    ++ctx->prepared_io;
    return {};
}

// IO URing prep ops 
AkIOOp ak_os_io_open(const char* path, int flags, mode_t mode) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);
    });
}

AkIOOp ak_os_io_open_at(int dfd, const char* path, int flags, mode_t mode) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_openat(sqe, dfd, path, flags, mode);
    });
}

AkIOOp ak_os_io_open_at_direct(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_openat_direct(sqe, dfd, path, flags, mode, file_index);
    });
}

AkIOOp ak_os_io_close(int fd) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_close(sqe, fd);
    });
}

AkIOOp ak_os_io_close_direct(unsigned file_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_close_direct(sqe, file_index);
    });
}

// Read Operations (definitions)
AkIOOp ak_os_io_read(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    });
}

AkIOOp ak_os_io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_read_multishot(sqe, fd, nbytes, offset, buf_group);
    });
}

AkIOOp ak_os_io_read_fixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

AkIOOp ak_os_io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
    });
}

AkIOOp ak_os_io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

AkIOOp ak_os_io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_readv_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Write Operations (definitions)
AkIOOp ak_os_io_write(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    });
}

AkIOOp ak_os_io_write_fixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    });
}

AkIOOp ak_os_io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
    });
}

AkIOOp ak_os_io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
    });
}

AkIOOp ak_os_io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_writev_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
    });
}

// Socket Operations (definitions)
AkIOOp ak_os_io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
    });
}

AkIOOp ak_os_io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_accept_direct(sqe, fd, addr, addrlen, flags, file_index);
    });
}

AkIOOp ak_os_io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept(sqe, fd, addr, addrlen, flags);
    });
}

AkIOOp ak_os_io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_multishot_accept_direct(sqe, fd, addr, addrlen, flags);
    });
}

AkIOOp ak_os_io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_connect(sqe, fd, addr, addrlen);
    });
}

AkIOOp ak_os_io_send(int sockfd, const void* buf, size_t len, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_send(sqe, sockfd, buf, len, flags);
    });
}

AkIOOp ak_os_io_send_zc(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc(sqe, sockfd, buf, len, flags, zc_flags);
    });
}

AkIOOp ak_os_io_send_zc_fixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, zc_flags, buf_index);
    });
}

AkIOOp ak_os_io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg(sqe, fd, msg, flags);
    });
}

AkIOOp ak_os_io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc(sqe, fd, msg, flags);
    });
}

AkIOOp ak_os_io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_sendmsg_zc_fixed(sqe, fd, msg, flags, buf_index);
    });
}

AkIOOp ak_os_io_recv(int sockfd, void* buf, size_t len, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    });
}

AkIOOp ak_os_io_recv_multishot(int sockfd, void* buf, size_t len, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_recv_multishot(sqe, sockfd, buf, len, flags);
    });
}

AkIOOp ak_os_io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, msg, flags);
    });
}

AkIOOp ak_os_io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_recvmsg_multishot(sqe, fd, msg, flags);
    });
}

AkIOOp ak_os_io_socket(int domain, int type, int protocol, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_socket(sqe, domain, type, protocol, flags);
    });
}

AkIOOp ak_os_io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_socket_direct(sqe, domain, type, protocol, file_index, flags);
    });
}

// Directory and Link Operations (definitions)
AkIOOp ak_os_io_mkdir(const char* path, mode_t mode) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdir(sqe, path, mode);
    });
}

AkIOOp ak_os_io_mkdir_at(int dfd, const char* path, mode_t mode) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_mkdirat(sqe, dfd, path, mode);
    });
}

AkIOOp ak_os_io_symlink(const char* target, const char* linkpath) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_symlink(sqe, target, linkpath);
    });
}

AkIOOp ak_os_io_symlink_at(const char* target, int newdirfd, const char* linkpath) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_symlinkat(sqe, target, newdirfd, linkpath);
    });
}

AkIOOp ak_os_io_link(const char* oldpath, const char* newpath, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_link(sqe, oldpath, newpath, flags);
    });
}

AkIOOp ak_os_io_link_at(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_linkat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}

// File Management Operations (definitions)
AkIOOp ak_os_io_unlink(const char* path, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_unlink(sqe, path, flags);
    });
}

AkIOOp ak_os_io_unlink_at(int dfd, const char* path, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_unlinkat(sqe, dfd, path, flags);
    });
}

AkIOOp ak_os_io_rename(const char* oldpath, const char* newpath) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_rename(sqe, oldpath, newpath);
    });
}

AkIOOp ak_os_io_rename_at(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_renameat(sqe, olddfd, oldpath, newdfd, newpath, flags);
    });
}
AkIOOp ak_os_io_sync(int fd, unsigned fsync_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fsync(sqe, fd, fsync_flags);
    });
}

AkIOOp ak_os_io_sync_file_range(int fd, unsigned len, __u64 offset, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_sync_file_range(sqe, fd, len, offset, flags);
    });
}

AkIOOp ak_os_io_fallocate(int fd, int mode, __u64 offset, __u64 len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fallocate(sqe, fd, mode, offset, len);
    });
}

AkIOOp ak_os_io_open_at2(int dfd, const char* path, struct open_how* how) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2(sqe, dfd, path, how);
    });
}

AkIOOp ak_os_io_open_at2_direct(int dfd, const char* path, struct open_how* how, unsigned file_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_openat2_direct(sqe, dfd, path, how, file_index);
    });
}

AkIOOp ak_os_io_statx(int dfd, const char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
    });
}

AkIOOp ak_os_io_fadvise(int fd, __u64 offset, __u32 len, int advice) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise(sqe, fd, offset, len, advice);
    });
}

AkIOOp ak_os_io_fadvise64(int fd, __u64 offset, off_t len, int advice) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fadvise64(sqe, fd, offset, len, advice);
    });
}

AkIOOp ak_os_io_madvise(void* addr, __u32 length, int advice) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise(sqe, addr, length, advice);
    });
}

AkIOOp ak_os_io_madvise64(void* addr, off_t length, int advice) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_madvise64(sqe, addr, length, advice);
    });
}

// Extended Attributes Operations
AkIOOp ak_os_io_get_xattr(const char* name, char* value, const char* path, unsigned int len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_getxattr(sqe, name, value, path, len);
    });
}

AkIOOp ak_os_io_set_xattr(const char* name, const char* value, const char* path, int flags, unsigned int len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_setxattr(sqe, name, value, path, flags, len);
    });
}

AkIOOp ak_os_io_fget_xattr(int fd, const char* name, char* value, unsigned int len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fgetxattr(sqe, fd, name, value, len);
    });
}

AkIOOp ak_os_io_fset_xattr(int fd, const char* name, const char* value, int flags, unsigned int len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fsetxattr(sqe, fd, name, value, flags, len);
    });
}

// Buffer Operations
AkIOOp ak_os_io_provide_buffers(void* addr, int len, int nr, int bgid, int bid) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
    });
}

AkIOOp ak_os_io_remove_buffers(int nr, int bgid) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_remove_buffers(sqe, nr, bgid);
    });
}

// Polling Operations
AkIOOp ak_os_io_poll_add(int fd, unsigned poll_mask) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_add(sqe, fd, poll_mask);
    });
}

AkIOOp ak_os_io_poll_multishot(int fd, unsigned poll_mask) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_multishot(sqe, fd, poll_mask);
    });
}

AkIOOp ak_os_io_poll_remove(__u64 user_data) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_remove(sqe, user_data);
    });
}

AkIOOp ak_os_io_poll_update(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_poll_update(sqe, old_user_data, new_user_data, poll_mask, flags);
    });
}

AkIOOp ak_os_io_epoll_ctl(int epfd, int fd, int op, struct epoll_event* ev) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_ctl(sqe, epfd, fd, op, ev);
    });
}

AkIOOp ak_os_io_epoll_wait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_epoll_wait(sqe, fd, events, maxevents, flags);
    });
}

// Timeout Operations
AkIOOp ak_os_io_timeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout(sqe, ts, count, flags);
    });
}

AkIOOp ak_os_io_timeout_remove(__u64 user_data, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_remove(sqe, user_data, flags);
    });
}

AkIOOp ak_os_io_timeout_update(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_timeout_update(sqe, ts, user_data, flags);
    });
}

AkIOOp ak_os_io_link_timeout(struct __kernel_timespec* ts, unsigned flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_link_timeout(sqe, ts, flags);
    });
}

// Message Ring Operations
AkIOOp ak_os_io_msg_ring(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring(sqe, fd, len, data, flags);
    });
}

AkIOOp ak_os_io_msg_ring_cqe_flags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_cqe_flags(sqe, fd, len, data, flags, cqe_flags);
    });
}

AkIOOp ak_os_io_msg_ring_fd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd(sqe, fd, source_fd, target_fd, data, flags);
    });
}

AkIOOp ak_os_io_msg_ring_fd_alloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_msg_ring_fd_alloc(sqe, fd, source_fd, data, flags);
    });
}

// Process Operations
AkIOOp ak_os_io_waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_waitid(sqe, idtype, id, infop, options, flags);
    });
}

// Futex Operations
AkIOOp ak_os_io_futex_wake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wake(sqe, futex, val, mask, futex_flags, flags);
    });
}

AkIOOp ak_os_io_futex_wait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_wait(sqe, futex, val, mask, futex_flags, flags);
    });
}

AkIOOp ak_os_io_futex_waitv(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_futex_waitv(sqe, futex, nr_futex, flags);
    });
}

// File Descriptor Management
AkIOOp ak_os_io_fixed_fd_install(int fd, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_fixed_fd_install(sqe, fd, flags);
    });
}

AkIOOp ak_os_io_files_update(int* fds, unsigned nr_fds, int offset) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_files_update(sqe, fds, nr_fds, offset);
    });
}

// Shutdown Operation
AkIOOp ak_os_io_shutdown(int fd, int how) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_shutdown(sqe, fd, how);
    });
}

// File Truncation
AkIOOp ak_os_io_ftruncate(int fd, loff_t len) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_ftruncate(sqe, fd, len);
    });
}

// Command Operations
AkIOOp ak_os_io_cmd_sock(int cmd_op, int fd, int level, int optname, void* optval, int optlen) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_sock(sqe, cmd_op, fd, level, optname, optval, optlen);
    });
}

AkIOOp ak_os_io_cmd_discard(int fd, uint64_t offset, uint64_t nbytes) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_cmd_discard(sqe, fd, offset, nbytes);
    });
}

// Special Operations
AkIOOp ak_os_io_nop(__u64 user_data) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_nop(sqe);
        sqe->off = user_data; // carry user data in an unused field for NOP
    });
}

// Splice Operations
AkIOOp ak_os_io_splice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
    });
}

AkIOOp ak_os_io_tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_tee(sqe, fd_in, fd_out, nbytes, splice_flags);
    });
}

// Cancel Operations
AkIOOp ak_os_io_cancel64(__u64 user_data, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel64(sqe, user_data, flags);
    });
}

AkIOOp ak_os_io_cancel(void* user_data, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel(sqe, user_data, flags);
    });
}

AkIOOp ak_os_io_cancel_fd(int fd, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_cancel_fd(sqe, fd, flags);
    });
}

// Additional convenience wrappers to cover liburing prep routines
AkIOOp ak_os_io_open_direct(const char* path, int flags, mode_t mode, unsigned file_index) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_open_direct(sqe, path, flags, mode, file_index);
    });
}

AkIOOp ak_os_io_send_bundle(int sockfd, size_t len, int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_send_bundle(sqe, sockfd, len, flags);
    });
}

AkIOOp ak_os_io_sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_sendto(sqe, sockfd, buf, len, flags, addr, addrlen);
    });
}

// Conditionally available wrappers depending on liburing feature macros
#if defined(IORING_FILE_INDEX_ALLOC)
AkIOOp ak_os_io_socket_direct_alloc(int domain, int type, int protocol, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_socket_direct_alloc(sqe, domain, type, protocol, flags);
    });
}
#endif

#if defined(IORING_OP_BIND)
AkIOOp ak_os_io_bind(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_rw(IORING_OP_BIND, sqe, fd, (void*)addr, 0, addrlen);
    });
}
#endif

#if defined(IORING_OP_LISTEN)
AkIOOp ak_os_io_listen(int fd, int backlog) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_rw(IORING_OP_LISTEN, sqe, fd, 0, backlog, 0);
    });
}
#endif

#if defined(IORING_OP_PIPE)
AkIOOp ak_os_io_pipe(int* fds, unsigned int flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_rw(IORING_OP_PIPE, sqe, 0, fds, 0, 0);
        sqe->rw_flags = flags;
    });
}

AkIOOp ak_os_io_pipe_direct(int* fds, unsigned int pipe_flags) noexcept {
    return prepare_io_uring_op([=](io_uring_sqe* sqe) {
        io_uring_prep_rw(IORING_OP_PIPE, sqe, 0, fds, 0, 0);
        sqe->rw_flags = pipe_flags;
    });
}
#endif
