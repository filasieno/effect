#pragma once

#include "ak/api_priv.hpp"

namespace ak { namespace priv {

    template <typename Prep>
    inline ExecIOOp PrepareIO(Prep prep) noexcept;

} }    

// Implementation
// ----------------------------------------------------------------------------------------------------------------

namespace ak {
    
    // ExecIOOp
    // ----------------------------------------------------------------------------------------------------------------

    constexpr TaskHdl ExecIOOp::await_suspend(TaskHdl currentTaskHdl) noexcept {
        // if suspend is called we know that the operation has been submitted
        using namespace priv;

        // Move the current Task from RUNNING to IO_WAITING
        TaskContext* ctx = &currentTaskHdl.promise();
        assert(ctx->state == TaskState::RUNNING);
        ctx->state = TaskState::IO_WAITING;
        ++gKernel.ioWaitingCount;
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();
        DebugTaskCount();

        // Move the scheduler task from READY to RUNNING
        TaskContext* schedCtx = &gKernel.schedulerTaskHdl.promise();
        assert(schedCtx->state == TaskState::READY);
        schedCtx->state = TaskState::RUNNING;
        DetachLink(&schedCtx->waitLink);
        --gKernel.readyCount;
        gKernel.currentTaskHdl = gKernel.schedulerTaskHdl;
        CheckInvariants();
        DebugTaskCount();

        return gKernel.schedulerTaskHdl;
    }

    // PrepareIO
    // ----------------------------------------------------------------------------------------------------------------

    namespace priv {
    
        template <typename Prep>
        inline ExecIOOp PrepareIO(Prep prep) noexcept {
            using namespace priv;
            
            TaskContext* ctx = &gKernel.currentTaskHdl.promise();
    
            // Ensure free submission slot 
            unsigned int free_slots = io_uring_sq_space_left(&gKernel.ioRing);
            while (free_slots < 1) {
                int ret = io_uring_submit(&gKernel.ioRing);
                if (ret < 0) {
                    abort();
                    // unreachable
                }
                free_slots = io_uring_sq_space_left(&gKernel.ioRing);
            }
    
            // Enqueue operation
            io_uring_sqe* sqe = io_uring_get_sqe(&gKernel.ioRing);
            io_uring_sqe_set_data(sqe, (void*) ctx);
            prep(sqe);  // Call the preparation function
            ctx->ioResult = 0;
            ++ctx->enqueuedIO;
            return {};
        }

    }

    // IO Operations
    // ----------------------------------------------------------------------------------------------------------------

    inline ExecIOOp IOOpen(const char* path, int flags, mode_t mode) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode);
        });
    }

    inline ExecIOOp IOOpenAt(int dfd, const char* path, int flags, mode_t mode) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_openat(sqe, dfd, path, flags, mode);
        });
    }

    inline ExecIOOp IOOpenAtDirect(int dfd, const char* path, int flags, mode_t mode, unsigned file_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_openat_direct(sqe, dfd, path, flags, mode, file_index);
        });
    }

    inline ExecIOOp IOClose(int fd) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_close(sqe, fd);
        });
    }

    inline ExecIOOp IOCloseDirect(unsigned file_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_close_direct(sqe, file_index);
        });
    }

    // Read Operations (definitions)
    inline ExecIOOp IORead(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        });
    }

    inline ExecIOOp IOReadMultishot(int fd, unsigned nbytes, __u64 offset, int buf_group) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_read_multishot(sqe, fd, nbytes, offset, buf_group);
        });
    }

    inline ExecIOOp IOReadFixed(int fd, void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        });
    }

    inline ExecIOOp IOReadV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
        });
    }

    inline ExecIOOp IOReadV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_readv2(sqe, fd, iovecs, nr_vecs, offset, flags);
        });
    }

    inline ExecIOOp IOReadVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_readv_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
        });
    }

    // Write Operations (definitions)
    inline ExecIOOp IOWrite(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        });
    }

    inline ExecIOOp IOWriteFixed(int fd, const void* buf, unsigned nbytes, __u64 offset, int buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
        });
    }

    inline ExecIOOp IOWriteV(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
        });
    }

    inline ExecIOOp IOWriteV2(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_writev2(sqe, fd, iovecs, nr_vecs, offset, flags);
        });
    }

    inline ExecIOOp IOWriteVFixed(int fd, const struct iovec* iovecs, unsigned nr_vecs, __u64 offset, int flags, int buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_writev_fixed(sqe, fd, iovecs, nr_vecs, offset, flags, buf_index);
        });
    }

    // Socket Operations (definitions)
    inline ExecIOOp IOAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        });
    }

    inline ExecIOOp IOAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags, unsigned int file_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_accept_direct(sqe, fd, addr, addrlen, flags, file_index);
        });
    }

    inline ExecIOOp IOMultishotAccept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_multishot_accept(sqe, fd, addr, addrlen, flags);
        });
    }

    inline ExecIOOp IOMultishotAcceptDirect(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_multishot_accept_direct(sqe, fd, addr, addrlen, flags);
        });
    }

    inline ExecIOOp IOConnect(int fd, const struct sockaddr* addr, socklen_t addrlen) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_connect(sqe, fd, addr, addrlen);
        });
    }

    inline ExecIOOp IOSend(int sockfd, const void* buf, size_t len, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_send(sqe, sockfd, buf, len, flags);
        });
    }

    inline ExecIOOp IOSendZC(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_send_zc(sqe, sockfd, buf, len, flags, zc_flags);
        });
    }

    inline ExecIOOp IOSendZCFixed(int sockfd, const void* buf, size_t len, int flags, unsigned zc_flags, unsigned buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_send_zc_fixed(sqe, sockfd, buf, len, flags, zc_flags, buf_index);
        });
    }

    inline ExecIOOp IOSendMsg(int fd, const struct msghdr* msg, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg(sqe, fd, msg, flags);
        });
    }

    inline ExecIOOp IOSendMsgZC(int fd, const struct msghdr* msg, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg_zc(sqe, fd, msg, flags);
        });
    }

    inline ExecIOOp IOSendMsgZCFixed(int fd, const struct msghdr* msg, unsigned flags, unsigned buf_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_sendmsg_zc_fixed(sqe, fd, msg, flags, buf_index);
        });
    }

    inline ExecIOOp IORecv(int sockfd, void* buf, size_t len, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_recv(sqe, sockfd, buf, len, flags);
        });
    }

    inline ExecIOOp IORecvMultishot(int sockfd, void* buf, size_t len, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_recv_multishot(sqe, sockfd, buf, len, flags);
        });
    }

    inline ExecIOOp IORecvMsg(int fd, struct msghdr* msg, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_recvmsg(sqe, fd, msg, flags);
        });
    }

    inline ExecIOOp IORecvMsgMultishot(int fd, struct msghdr* msg, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_recvmsg_multishot(sqe, fd, msg, flags);
        });
    }

    inline ExecIOOp IOSocket(int domain, int type, int protocol, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_socket(sqe, domain, type, protocol, flags);
        });
    }

    inline ExecIOOp IOSocketDirect(int domain, int type, int protocol, unsigned file_index, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_socket_direct(sqe, domain, type, protocol, file_index, flags);
        });
    }

    // Directory and Link Operations (definitions)
    inline ExecIOOp IOMkdir(const char* path, mode_t mode) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_mkdir(sqe, path, mode);
        });
    }

    inline ExecIOOp IOMkdirAt(int dfd, const char* path, mode_t mode) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_mkdirat(sqe, dfd, path, mode);
        });
    }

    inline ExecIOOp IOSymlink(const char* target, const char* linkpath) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_symlink(sqe, target, linkpath);
        });
    }

    inline ExecIOOp IOSymlinkAt(const char* target, int newdirfd, const char* linkpath) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_symlinkat(sqe, target, newdirfd, linkpath);
        });
    }

    inline ExecIOOp IOLink(const char* oldpath, const char* newpath, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_link(sqe, oldpath, newpath, flags);
        });
    }

    inline ExecIOOp IOLinkAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_linkat(sqe, olddfd, oldpath, newdfd, newpath, flags);
        });
    }

    // File Management Operations (definitions)
    inline ExecIOOp IOUnlink(const char* path, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_unlink(sqe, path, flags);
        });
    }

    inline ExecIOOp IOUnlinkAt(int dfd, const char* path, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_unlinkat(sqe, dfd, path, flags);
        });
    }

    inline ExecIOOp IORename(const char* oldpath, const char* newpath) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_rename(sqe, oldpath, newpath);
        });
    }

    inline ExecIOOp IORenameAt(int olddfd, const char* oldpath, int newdfd, const char* newpath, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_renameat(sqe, olddfd, oldpath, newdfd, newpath, flags);
        });
    }
    inline ExecIOOp IOSync(int fd, unsigned fsync_flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fsync(sqe, fd, fsync_flags);
        });
    }

    inline ExecIOOp IOSyncFileRange(int fd, unsigned len, __u64 offset, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_sync_file_range(sqe, fd, len, offset, flags);
        });
    }

    inline ExecIOOp IOFAllocate(int fd, int mode, __u64 offset, __u64 len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fallocate(sqe, fd, mode, offset, len);
        });
    }

    inline ExecIOOp IOOpenAt2(int dfd, const char* path, struct open_how* how) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_openat2(sqe, dfd, path, how);
        });
    }

    inline ExecIOOp IOOpenAt2Direct(int dfd, const char* path, struct open_how* how, unsigned file_index) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_openat2_direct(sqe, dfd, path, how, file_index);
        });
    }

    inline ExecIOOp IOStatx(int dfd, const char* path, int flags, unsigned mask, struct statx* statxbuf) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_statx(sqe, dfd, path, flags, mask, statxbuf);
        });
    }

    inline ExecIOOp IOFAdvise(int fd, __u64 offset, __u32 len, int advice) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fadvise(sqe, fd, offset, len, advice);
        });
    }

    inline ExecIOOp IOFAdvise64(int fd, __u64 offset, off_t len, int advice) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fadvise64(sqe, fd, offset, len, advice);
        });
    }

    inline ExecIOOp IOMAdvise(void* addr, __u32 length, int advice) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_madvise(sqe, addr, length, advice);
        });
    }

    inline ExecIOOp IOMAdvise64(void* addr, off_t length, int advice) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_madvise64(sqe, addr, length, advice);
        });
    }

    // Extended Attributes Operations
    inline ExecIOOp IOGetXAttr(const char* name, char* value, const char* path, unsigned int len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_getxattr(sqe, name, value, path, len);
        });
    }

    inline ExecIOOp IOSetXAttr(const char* name, const char* value, const char* path, int flags, unsigned int len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_setxattr(sqe, name, value, path, flags, len);
        });
    }

    inline ExecIOOp IOFGetXAttr(int fd, const char* name, char* value, unsigned int len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fgetxattr(sqe, fd, name, value, len);
        });
    }

    inline ExecIOOp IOFSetXAttr(int fd, const char* name, const char* value, int flags, unsigned int len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fsetxattr(sqe, fd, name, value, flags, len);
        });
    }

    // Buffer Operations
    inline ExecIOOp IOProvideBuffers(void* addr, int len, int nr, int bgid, int bid) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_provide_buffers(sqe, addr, len, nr, bgid, bid);
        });
    }

    inline ExecIOOp IORemoveBuffers(int nr, int bgid) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_remove_buffers(sqe, nr, bgid);
        });
    }

    // Polling Operations
    inline ExecIOOp IOPollAdd(int fd, unsigned poll_mask) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_add(sqe, fd, poll_mask);
        });
    }

    inline ExecIOOp IOPollMultishot(int fd, unsigned poll_mask) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_multishot(sqe, fd, poll_mask);
        });
    }

    inline ExecIOOp IOPollRemove(__u64 user_data) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_remove(sqe, user_data);
        });
    }

    inline ExecIOOp IOPollUpdate(__u64 old_user_data, __u64 new_user_data, unsigned poll_mask, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_poll_update(sqe, old_user_data, new_user_data, poll_mask, flags);
        });
    }

    inline ExecIOOp IOEpollCtl(int epfd, int fd, int op, struct epoll_event* ev) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_epoll_ctl(sqe, epfd, fd, op, ev);
        });
    }

    inline ExecIOOp IOEpollWait(int fd, struct epoll_event* events, int maxevents, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_epoll_wait(sqe, fd, events, maxevents, flags);
        });
    }

    // Timeout Operations
    inline ExecIOOp IOTimeout(struct __kernel_timespec* ts, unsigned count, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout(sqe, ts, count, flags);
        });
    }

    inline ExecIOOp IOTimeoutRemove(__u64 user_data, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout_remove(sqe, user_data, flags);
        });
    }

    inline ExecIOOp IOTimeoutUpdate(struct __kernel_timespec* ts, __u64 user_data, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_timeout_update(sqe, ts, user_data, flags);
        });
    }

    inline ExecIOOp IOLinkTimeout(struct __kernel_timespec* ts, unsigned flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_link_timeout(sqe, ts, flags);
        });
    }

    // Message Ring Operations
    inline ExecIOOp IOMsgRing(int fd, unsigned int len, __u64 data, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring(sqe, fd, len, data, flags);
        });
    }

    inline ExecIOOp IOMsgRingCqeFlags(int fd, unsigned int len, __u64 data, unsigned int flags, unsigned int cqe_flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_cqe_flags(sqe, fd, len, data, flags, cqe_flags);
        });
    }

    inline ExecIOOp IOMsgRingFd(int fd, int source_fd, int target_fd, __u64 data, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_fd(sqe, fd, source_fd, target_fd, data, flags);
        });
    }

    inline ExecIOOp IOMsgRingFdAlloc(int fd, int source_fd, __u64 data, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_msg_ring_fd_alloc(sqe, fd, source_fd, data, flags);
        });
    }

    // Process Operations
    inline ExecIOOp IOWaitId(idtype_t idtype, id_t id, siginfo_t* infop, int options, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_waitid(sqe, idtype, id, infop, options, flags);
        });
    }

    // Futex Operations
    inline ExecIOOp IOFutexWake(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_wake(sqe, futex, val, mask, futex_flags, flags);
        });
    }

    inline ExecIOOp IOFutexWait(uint32_t* futex, uint64_t val, uint64_t mask, uint32_t futex_flags, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_wait(sqe, futex, val, mask, futex_flags, flags);
        });
    }

    inline ExecIOOp IOFutexWaitV(struct futex_waitv* futex, uint32_t nr_futex, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_futex_waitv(sqe, futex, nr_futex, flags);
        });
    }

    // File Descriptor Management
    inline ExecIOOp IOFixedFdInstall(int fd, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_fixed_fd_install(sqe, fd, flags);
        });
    }

    inline ExecIOOp IOFilesUpdate(int* fds, unsigned nr_fds, int offset) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_files_update(sqe, fds, nr_fds, offset);
        });
    }

    // Shutdown Operation
    inline ExecIOOp IOShutdown(int fd, int how) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_shutdown(sqe, fd, how);
        });
    }

    // File Truncation
    inline ExecIOOp IOFTruncate(int fd, loff_t len) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_ftruncate(sqe, fd, len);
        });
    }

    // Command Operations
    inline ExecIOOp IOCmdSock(int cmd_op, int fd, int level, int optname, void* optval, int optlen) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_cmd_sock(sqe, cmd_op, fd, level, optname, optval, optlen);
        });
    }

    inline ExecIOOp IOCmdDiscard(int fd, uint64_t offset, uint64_t nbytes) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_cmd_discard(sqe, fd, offset, nbytes);
        });
    }

    // Special Operations
    inline ExecIOOp IONop() noexcept {
        return priv::PrepareIO([](io_uring_sqe* sqe) {
            io_uring_prep_nop(sqe);
        });
    }

    // Splice Operations
    inline ExecIOOp IOSplice(int fd_in, int64_t off_in, int fd_out, int64_t off_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_splice(sqe, fd_in, off_in, fd_out, off_out, nbytes, splice_flags);
        });
    }

    inline ExecIOOp IOTee(int fd_in, int fd_out, unsigned int nbytes, unsigned int splice_flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_tee(sqe, fd_in, fd_out, nbytes, splice_flags);
        });
    }

    // Cancel Operations
    inline ExecIOOp IOCancel64(__u64 user_data, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel64(sqe, user_data, flags);
        });
    }

    inline ExecIOOp IOCancel(void* user_data, int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel(sqe, user_data, flags);
        });
    }

    inline ExecIOOp IOCancelFd(int fd, unsigned int flags) noexcept {
        return priv::PrepareIO([=](io_uring_sqe* sqe) {
            io_uring_prep_cancel_fd(sqe, fd, flags);
        });
    }

}
