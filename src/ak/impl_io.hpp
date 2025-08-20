#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {

    template <typename Prep>
    inline op::ExecIO prep_io(Prep prep) noexcept;

} }    

// Implementation
// ----------------------------------------------------------------------------------------------------------------

namespace ak {
    
    // ExecIOOp
    // ----------------------------------------------------------------------------------------------------------------

    constexpr CThread::Hdl op::ExecIO::await_suspend(CThread::Hdl current_context_hdl) noexcept {
        // if suspend is called we know that the operation has been submitted
        using namespace priv;
        

        // Move the current Task from RUNNING to IO_WAITING
        auto* current_context = get_context(current_context_hdl);
        assert(current_context->state == CThread::State::RUNNING);
        current_context->state = CThread::State::IO_WAITING;
        ++global_kernel_state.iowaiting_cthread_count;
        global_kernel_state.current_cthread.reset();
        check_invariants();
        dump_task_count();

        // Move the scheduler task from READY to RUNNING
        auto* sched_ctx = get_context(global_kernel_state.scheduler_cthread);
        assert(sched_ctx->state == CThread::State::READY);
        sched_ctx->state = CThread::State::RUNNING;
        utl::detach_dlink(&sched_ctx->wait_link);
        --global_kernel_state.ready_cthread_count;
        global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
        check_invariants();
        dump_task_count();

        return global_kernel_state.scheduler_cthread;
    }

    // PrepareIO
    // ----------------------------------------------------------------------------------------------------------------

    namespace priv {
    
        template <typename PrepFn>
        inline op::ExecIO prep_io(PrepFn prep_fn) noexcept {
            using namespace priv;
            
            CThread::Context* ctx = get_context(global_kernel_state.current_cthread);
    
            // Ensure free submission slot 
            unsigned int free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
            while (free_slots < 1) {
                int ret = io_uring_submit(&global_kernel_state.io_uring_state);
                if (ret < 0) {
                    abort();
                    // unreachable
                }
                free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
            }
    
            // Enqueue operation
            io_uring_sqe* sqe = io_uring_get_sqe(&global_kernel_state.io_uring_state);
            io_uring_sqe_set_data(sqe, (Void*) ctx);
            prep_fn(sqe);  // Call the preparation function
            ctx->res = 0;
            ++ctx->prepared_io;
            return {};
        }

    }

    // IO Operations
    // ----------------------------------------------------------------------------------------------------------------

    inline op::ExecIO io_open(const Char* path, int flags, mode_t mode) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);
        });
    }

    inline op::ExecIO io_open_at(int dfd, const Char* path, int flags, mode_t mode) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_openat(sqe, dfd, path, flags, mode);
        });
    }

    inline op::ExecIO io_open_at_direct(int dfd, const Char* path, int flags, mode_t mode, unsigned file_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_openat_direct(sqe, dfd, path, flags, mode, file_index);
        });
    }

    inline op::ExecIO io_close(int fd) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_close(sqe, fd);
        });
    }

    inline op::ExecIO io_close_direct(unsigned file_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_close_direct(sqe, file_index);
        });
    }

    // Read Operations (definitions)
    inline op::ExecIO io_read(int fd, Void* buf, unsigned nbytes, __u64 offset) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        });
    }

    inline op::ExecIO io_read_multishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_read_multishot(sqe, fd, nbytes, offset, buf_group);
        });
    }

    inline op::ExecIO io_read_fixed(int fd, Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        });
    }

    inline op::ExecIO io_readv(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        });
    }

    inline op::ExecIO io_readv2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
        });
    }

    inline op::ExecIO io_readv_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_readv_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
        });
    }

    // Write Operations (definitions)
    inline op::ExecIO io_write(int fd, const Void* buf, unsigned nbytes, __u64 offset) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        });
    }

    inline op::ExecIO io_write_fixed(int fd, const Void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        });
    }

    inline op::ExecIO io_writev(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        });
    }

    inline op::ExecIO io_writev2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
        });
    }

    inline op::ExecIO io_writev_fixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_writev_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
        });
    }

    // Socket Operations (definitions)
    inline op::ExecIO io_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        });
    }

    inline op::ExecIO io_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_accept_direct(sqe, fd, addr, addrlen, flags, file_index);
        });
    }

    inline op::ExecIO io_multishot_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_multishot_accept(sqe, fd, addr, addrlen, flags);
        });
    }

    inline op::ExecIO io_multishot_accept_direct(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_multishot_accept_direct(sqe, fd, addr, addrlen, flags);
        });
    }

    inline op::ExecIO io_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        });
    }

    inline op::ExecIO io_send(int sockfd, const Void* buf, size_t len, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_send(sqe, sockfd, buf, len, flags);
        });
    }

    inline op::ExecIO io_send_zc(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_send_zc(sqe, sockfd, buf, len, flags, zc_flags);
        });
    }

    inline op::ExecIO io_send_zc_fixed(int sockfd, const Void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, zc_flags, buf_index);
        });
    }

    inline op::ExecIO io_send_msg(int fd, const struct msghdr* msg, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg(sqe, fd, msg, flags);
        });
    }

    inline op::ExecIO io_send_msg_zc(int fd, const struct msghdr* msg, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg_zc(sqe, fd, msg, flags);
        });
    }

    inline op::ExecIO io_send_msg_zc_fixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg_zc_fixed(sqe, fd, msg, flags, buf_index);
        });
    }

    inline op::ExecIO io_recv(int sockfd, Void* buf, size_t len, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_recv(sqe, sockfd, buf, len, flags);
        });
    }

    inline op::ExecIO io_recv_multishot(int sockfd, Void* buf, size_t len, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_recv_multishot(sqe, sockfd, buf, len, flags);
        });
    }

    inline op::ExecIO io_recv_msg(int fd, struct msghdr* msg, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_recvmsg(sqe, fd, msg, flags);
        });
    }

    inline op::ExecIO io_recv_msg_multishot(int fd, struct msghdr* msg, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_recvmsg_multishot(sqe, fd, msg, flags);
        });
    }

    inline op::ExecIO io_socket(int domain, int type, int protocol, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_socket(sqe, domain, type, protocol, flags);
        });
    }

    inline op::ExecIO io_socket_direct(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_socket_direct(sqe, domain, type, protocol, file_index, flags);
        });
    }

    // Directory and Link Operations (definitions)
    inline op::ExecIO io_mkdir(const Char* path, mode_t mode) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_mkdir(sqe, path, mode);
        });
    }

    inline op::ExecIO io_mkdir_at(int dfd, const Char* path, mode_t mode) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_mkdirat(sqe, dfd, path, mode);
        });
    }

    inline op::ExecIO io_symlink(const Char* target, const Char* linkpath) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_symlink(sqe, target, linkpath);
        });
    }

    inline op::ExecIO io_symlink_at(const Char* target, int newdirfd, const Char* linkpath) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_symlinkat(sqe, target, newdirfd, linkpath);
        });
    }

    inline op::ExecIO io_link(const Char* oldpath, const Char* newpath, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_link(sqe, oldpath, newpath, flags);
        });
    }

    inline op::ExecIO io_link_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_linkat(sqe, olddfd, oldpath, newdfd, newpath, flags);
        });
    }

    // File Management Operations (definitions)
    inline op::ExecIO io_unlink(const Char* path, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_unlink(sqe, path, flags);
        });
    }

    inline op::ExecIO io_unlink_at(int dfd, const Char* path, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_unlinkat(sqe, dfd, path, flags);
        });
    }

    inline op::ExecIO io_rename(const Char* oldpath, const Char* newpath) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_rename(sqe, oldpath, newpath);
        });
    }

    inline op::ExecIO io_rename_at(int olddfd, const Char* oldpath, int newdfd, const Char* newpath, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_renameat(sqe, olddfd, oldpath, newdfd, newpath, flags);
        });
    }
    inline op::ExecIO io_sync(int fd, unsigned fsync_flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fsync(sqe, fd, fsync_flags);
        });
    }

    inline op::ExecIO io_sync_file_range(int fd, unsigned len, __u64 offset, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_sync_file_range(sqe, fd, len, offset, flags);
        });
    }

    inline op::ExecIO io_fallocate(int fd, int mode, __u64 offset, __u64 len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fallocate(sqe, fd, mode, offset, len);
        });
    }

    inline op::ExecIO io_open_at2(int dfd, const Char* path, struct open_how* how) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_openat2(sqe, dfd, path, how);
        });
    }

    inline op::ExecIO IOOpenAt2Direct(int dfd, const Char* path, struct open_how* how, unsigned file_index) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_openat2_direct(sqe, dfd, path, how, file_index);
        });
    }

    inline op::ExecIO IOStatx(int dfd, const Char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
        });
    }

    inline op::ExecIO IOFAdvise(int fd, __u64 offset, __u32 len, int advice) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fadvise(sqe, fd, offset, len, advice);
        });
    }

    inline op::ExecIO IOFAdvise64(int fd, __u64 offset, off_t len, int advice) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fadvise64(sqe, fd, offset, len, advice);
        });
    }

    inline op::ExecIO IOMAdvise(Void* addr, __u32 length, int advice) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_madvise(sqe, addr, length, advice);
        });
    }

    inline op::ExecIO IOMAdvise64(Void* addr, off_t length, int advice) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_madvise64(sqe, addr, length, advice);
        });
    }

    // Extended Attributes Operations
    inline op::ExecIO IOGetXAttr(const Char* name, Char* value, const Char* path, unsigned int len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_getxattr(sqe, name, value, path, len);
        });
    }

    inline op::ExecIO IOSetXAttr(const Char* name, const Char* value, const Char* path, int flags, unsigned int len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_setxattr(sqe, name, value, path, flags, len);
        });
    }

    inline op::ExecIO IOFGetXAttr(int fd, const Char* name, Char* value, unsigned int len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fgetxattr(sqe, fd, name, value, len);
        });
    }

    inline op::ExecIO IOFSetXAttr(int fd, const Char* name, const Char* value, int flags, unsigned int len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fsetxattr(sqe, fd, name, value, flags, len);
        });
    }

    // Buffer Operations
    inline op::ExecIO IOProvideBuffers(Void* addr, int len, int nr, int bgid, int bid) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
        });
    }

    inline op::ExecIO IORemoveBuffers(int nr, int bgid) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_remove_buffers(sqe, nr, bgid);
        });
    }

    // Polling Operations
    inline op::ExecIO IOPollAdd(int fd, unsigned poll_mask) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_add(sqe, fd, poll_mask);
        });
    }

    inline op::ExecIO IOPollMultishot(int fd, unsigned poll_mask) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_multishot(sqe, fd, poll_mask);
        });
    }

    inline op::ExecIO IOPollRemove(__u64 user_data) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_remove(sqe, user_data);
        });
    }

    inline op::ExecIO IOPollUpdate(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_update(sqe, old_user_data, new_user_data, poll_mask, flags);
        });
    }

    inline op::ExecIO IOEpollCtl(int epfd, int fd, int op, struct epoll_event* ev) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_epoll_ctl(sqe, epfd, fd, op, ev);
        });
    }

    inline op::ExecIO IOEpollWait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_epoll_wait(sqe, fd, events, maxevents, flags);
        });
    }

    // Timeout Operations
    inline op::ExecIO IOTimeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout(sqe, ts, count, flags);
        });
    }

    inline op::ExecIO IOTimeoutRemove(__u64 user_data, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout_remove(sqe, user_data, flags);
        });
    }

    inline op::ExecIO IOTimeoutUpdate(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout_update(sqe, ts, user_data, flags);
        });
    }

    inline op::ExecIO IOLinkTimeout(struct __kernel_timespec* ts, unsigned flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_link_timeout(sqe, ts, flags);
        });
    }

    // Message Ring Operations
    inline op::ExecIO IOMsgRing(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring(sqe, fd, len, data, flags);
        });
    }

    inline op::ExecIO IOMsgRingCqeFlags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_cqe_flags(sqe, fd, len, data, flags, cqe_flags);
        });
    }

    inline op::ExecIO IOMsgRingFd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_fd(sqe, fd, source_fd, target_fd, data, flags);
        });
    }

    inline op::ExecIO IOMsgRingFdAlloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_fd_alloc(sqe, fd, source_fd, data, flags);
        });
    }

    // Process Operations
    inline op::ExecIO IOWaitId(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_waitid(sqe, idtype, id, infop, options, flags);
        });
    }

    // Futex Operations
    inline op::ExecIO IOFutexWake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_wake(sqe, futex, val, mask, futex_flags, flags);
        });
    }

    inline op::ExecIO IOFutexWait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_wait(sqe, futex, val, mask, futex_flags, flags);
        });
    }

    inline op::ExecIO IOFutexWaitV(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_waitv(sqe, futex, nr_futex, flags);
        });
    }

    // File Descriptor Management
    inline op::ExecIO IOFixedFdInstall(int fd, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_fixed_fd_install(sqe, fd, flags);
        });
    }

    inline op::ExecIO IOFilesUpdate(int* fds, unsigned nr_fds, int offset) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_files_update(sqe, fds, nr_fds, offset);
        });
    }

    // Shutdown Operation
    inline op::ExecIO IOShutdown(int fd, int how) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_shutdown(sqe, fd, how);
        });
    }

    // File Truncation
    inline op::ExecIO IOFTruncate(int fd, loff_t len) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_ftruncate(sqe, fd, len);
        });
    }

    // Command Operations
    inline op::ExecIO IOCmdSock(int cmd_op, int fd, int level, int optname, Void* optval, int optlen) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_cmd_sock(sqe, cmd_op, fd, level, optname, optval, optlen);
        });
    }

    inline op::ExecIO IOCmdDiscard(int fd, uint64_t offset, uint64_t nbytes) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_cmd_discard(sqe, fd, offset, nbytes);
        });
    }

    // Special Operations
    inline op::ExecIO IONop() noexcept {
        return priv::prep_io([](io_uring_sqe* sqe) {
            io_uring_prep_nop(sqe);
        });
    }

    // Splice Operations
    inline op::ExecIO IOSplice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
        });
    }

    inline op::ExecIO IOTee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_tee(sqe, fd_in, fd_out, nbytes, splice_flags);
        });
    }

    // Cancel Operations
    inline op::ExecIO IOCancel64(__u64 user_data, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel64(sqe, user_data, flags);
        });
    }

    inline op::ExecIO IOCancel(Void* user_data, int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel(sqe, user_data, flags);
        });
    }

    inline op::ExecIO IOCancelFd(int fd, unsigned int flags) noexcept {
        return priv::prep_io([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel_fd(sqe, fd, flags);
        });
    }

}
