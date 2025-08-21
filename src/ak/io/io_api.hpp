// #pragma once

// // DEPRECATED: IO API moved to runtime_api.hpp
// #include "ak/runtime/runtime.hpp" // IWYU pragma: keep

// namespace ak {

//     namespace op {
//         struct ExecIO {
//             using Hdl = CThread::Hdl;
            
//             constexpr Bool await_ready() const noexcept  { return false; }
//             constexpr I32  await_resume() const noexcept { return global_kernel_state.current_cthread.hdl.promise().res; }
//             constexpr Hdl  await_suspend(Hdl hdl) noexcept;
//         };
//     }

//     // IO Routines
//     op::ExecIO io_open(const Char* path, int flags, mode_t mode) noexcept;
//     op::ExecIO io_open_at(int dfd, const Char* path, int flags, mode_t mode) noexcept;
//     op::ExecIO io_open_at_direct(int dfd, const Char* path, int flags, mode_t mode, unsigned file_index) noexcept;
//     op::ExecIO io_open_at2(int dfd, const Char* path, struct open_how* how) noexcept;
//     op::ExecIO io_open_at2_direct(int dfd, const Char* path, struct open_how* how, unsigned file_index) noexcept;
//     op::ExecIO io_open_direct(const Char* path, int flags, mode_t mode, unsigned file_index) noexcept;
//     op::ExecIO io_close(int fd) noexcept;
//     op::ExecIO io_close_direct(unsigned file_index) noexcept;
//     op::ExecIO io_read(int fd, Void* buf, unsigned nbytes, __u64 offset) noexcept;
//     op::ExecIO io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept;
//     op::ExecIO io_read_fixed(int fd, Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
//     op::ExecIO io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
//     op::ExecIO io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
//     op::ExecIO io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
//     op::ExecIO io_write(int fd, const Void* buf, unsigned nbytes, __u64 offset) noexcept;
//     op::ExecIO io_write_fixed(int fd, const Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept;
//     op::ExecIO io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept;
//     op::ExecIO io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept;
//     op::ExecIO io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept;
//     op::ExecIO io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
//     op::ExecIO io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept;
//     op::ExecIO io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
//     op::ExecIO io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept;
//     op::ExecIO io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
//     #if defined(IORING_OP_BIND)
//     op::ExecIO io_bind(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept;
//     #endif
//     #if defined(IORING_OP_LISTEN)
//     op::ExecIO io_listen(int fd, int backlog) noexcept;
//     #endif
//     op::ExecIO io_send(int sockfd, const Void* buf, size_t len, int flags) noexcept;
//     op::ExecIO io_send_bundle(int sockfd, size_t len, int flags) noexcept;
//     op::ExecIO io_sendto(int sockfd, const Void* buf, size_t len, int flags, const struct sockaddr* addr, socklen_t addrlen) noexcept;
//     op::ExecIO io_send_zc(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags) noexcept;
//     op::ExecIO io_send_zc_fixed(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept;
//     op::ExecIO io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept;
//     op::ExecIO io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept;
//     op::ExecIO io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept;
//     op::ExecIO io_recv(int sockfd, Void* buf, size_t len, int flags) noexcept;
//     op::ExecIO io_recv_multishot(int sockfd, Void* buf, size_t len, int flags) noexcept;
//     op::ExecIO io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept;
//     op::ExecIO io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept;
//     op::ExecIO io_socket(int domain, int type, int protocol, unsigned int flags) noexcept;
//     op::ExecIO io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept;
//     #if defined(IORING_FILE_INDEX_ALLOC)
//     op::ExecIO io_socket_direct_alloc(int domain, int type, int protocol, unsigned int flags) noexcept;
//     #endif
//     #if defined(IORING_OP_PIPE)
//     op::ExecIO io_pipe(int* fds, unsigned int flags) noexcept;
//     op::ExecIO io_pipe_direct(int* fds, unsigned int pipe_flags) noexcept;
//     #endif
//     op::ExecIO io_mkdir(const Char* path, mode_t mode) noexcept;
//     op::ExecIO io_mkdir_at(int dfd, const Char* path, mode_t mode) noexcept;
//     op::ExecIO io_symlink(const Char* target, const Char* linkpath) noexcept;
//     op::ExecIO io_symlink_at(const Char* target, int newdirfd, const Char* linkpath) noexcept;
//     op::ExecIO io_link(const Char* oldpath, const Char* newpath, int flags) noexcept;
//     op::ExecIO io_link_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, int flags) noexcept;
//     op::ExecIO io_unlink(const Char* path, int flags) noexcept;
//     op::ExecIO io_unlink_at(int dfd, const Char* path, int flags) noexcept;
//     op::ExecIO io_rename(const Char* oldpath, const Char* newpath) noexcept;
//     op::ExecIO io_rename_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, unsigned int flags) noexcept;
//     op::ExecIO io_sync(int fd, unsigned fsync_flags) noexcept;
//     op::ExecIO io_sync_file_range(int fd, unsigned len, __u64 offset, int flags) noexcept;
//     op::ExecIO io_fallocate(int fd, int mode, __u64 offset, __u64 len) noexcept;
//     op::ExecIO io_statx(int dfd, const Char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept;
//     op::ExecIO io_fadvise(int fd, __u64 offset, __u32 len, int advice) noexcept;
//     op::ExecIO io_fadvise64(int fd, __u64 offset, off_t len, int advice) noexcept;
//     op::ExecIO io_madvise(Void* addr, __u32 length, int advice) noexcept;
//     op::ExecIO io_madvise64(Void* addr, off_t length, int advice) noexcept;
//     op::ExecIO io_get_xattr(const Char* name, Char* value, const Char* path, unsigned int len) noexcept;
//     op::ExecIO io_set_xattr(const Char* name, const Char* value, const Char* path, int flags, unsigned int len) noexcept;
//     op::ExecIO io_fget_xattr(int fd, const Char* name, Char* value, unsigned int len) noexcept;
//     op::ExecIO io_fset_xattr(int fd, const Char* name, const Char* value, int flags, unsigned int len) noexcept;
//     op::ExecIO io_provide_buffers(Void* addr, int len, int nr, int bgid, int bid) noexcept;
//     op::ExecIO io_remove_buffers(int nr, int bgid) noexcept;
//     op::ExecIO io_poll_add(int fd, unsigned poll_mask) noexcept;
//     op::ExecIO io_poll_multishot(int fd, unsigned poll_mask) noexcept;
//     op::ExecIO io_poll_remove(__u64 user_data) noexcept;
//     op::ExecIO io_poll_update(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept;
//     op::ExecIO io_epoll_ctl(int epfd, int fd, int op, struct epoll_event* ev) noexcept;
//     op::ExecIO io_epoll_wait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept;
//     op::ExecIO io_timeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept;
//     op::ExecIO io_timeout_remove(__u64 user_data, unsigned flags) noexcept;
//     op::ExecIO io_timeout_update(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept;
//     op::ExecIO io_link_timeout(struct __kernel_timespec* ts, unsigned flags) noexcept;
//     op::ExecIO io_msg_ring(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept;
//     op::ExecIO io_msg_ring_cqe_flags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept;
//     op::ExecIO io_msg_ring_fd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept;
//     op::ExecIO io_msg_ring_fd_alloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept;
//     op::ExecIO io_waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept;
//     op::ExecIO io_futex_wake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
//     op::ExecIO io_futex_wait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept;
//     op::ExecIO io_futex_waitv(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept;
//     op::ExecIO io_fixed_fd_install(int fd, unsigned int flags) noexcept;
//     op::ExecIO io_files_update(int* fds, unsigned nr_fds, int offset) noexcept;
//     op::ExecIO io_shutdown(int fd, int how) noexcept;
//     op::ExecIO io_ftruncate(int fd, loff_t len) noexcept;
//     op::ExecIO io_cmd_sock(int cmd_op, int fd, int level, int optname, Void* optval, int optlen) noexcept;
//     op::ExecIO io_cmd_discard(int fd, uint64_t offset, uint64_t nbytes) noexcept;
//     op::ExecIO io_nop(__u64 user_data) noexcept;
//     op::ExecIO io_splice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
//     op::ExecIO io_tee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept;
//     op::ExecIO io_cancel64(__u64 user_data, int flags) noexcept;
//     op::ExecIO io_cancel(Void* user_data, int flags) noexcept;
//     op::ExecIO io_cancel_fd(int fd, unsigned int flags) noexcept;
// }