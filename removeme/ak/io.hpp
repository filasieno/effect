#pragma once

#include "ak/core.hpp"

namespace ak {

inline int GetCurrentTaskEnqueuedIOOps() noexcept {
    return GetTaskContext()->enqueuedIO;
}

namespace internal {

struct IODrainOp {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept { return currentTaskHdl; }
    constexpr int await_resume() const noexcept { return 0; }
};

struct IOWaitOneOp {
    constexpr bool await_ready() const noexcept { return false; }
    constexpr TaskHdl await_suspend(TaskHdl currentTaskHdl) noexcept {
        using namespace internal;
        TaskContext* ctx = &currentTaskHdl.promise();
        assert(ctx->state == TaskState::RUNNING);
        ctx->state = TaskState::IO_WAITING;
        ++gKernel.ioWaitingCount;
        ClearTaskHdl(&gKernel.currentTaskHdl);
        CheckInvariants();
        DebugTaskCount();
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
    constexpr int await_resume() const noexcept { return gKernel.currentTaskHdl.promise().ioResult; }
};

template <typename Prep>
inline internal::IOWaitOneOp PrepareIO(Prep prep) noexcept {
    using namespace internal;
    TaskContext* ctx = &gKernel.currentTaskHdl.promise();
    unsigned int free_slots = io_uring_sq_space_left(&gKernel.ioRing);
    while (free_slots < 1) {
        int ret = io_uring_submit(&gKernel.ioRing);
        if (ret < 0) { abort(); }
        free_slots = io_uring_sq_space_left(&gKernel.ioRing);
    }
    io_uring_sqe* sqe = io_uring_get_sqe(&gKernel.ioRing);
    io_uring_sqe_set_data(sqe, (void*) ctx);
    prep(sqe);
    ctx->ioResult = 0;
    ++ctx->enqueuedIO;
    return {};
}

} // namespace internal

// A curated subset shown here; the rest follow the same pattern and should be moved as needed.
inline internal::IOWaitOneOp IOOpen(const char* path, int flags, mode_t mode) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) { io_uring_prep_openat(sqe, AT_FDCWD, path, flags, mode); });
}
inline internal::IOWaitOneOp IOClose(int fd) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) { io_uring_prep_close(sqe, fd); });
}
inline internal::IOWaitOneOp IORead(int fd, void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) { io_uring_prep_read(sqe, fd, buf, nbytes, offset); });
}
inline internal::IOWaitOneOp IOWrite(int fd, const void* buf, unsigned nbytes, __u64 offset) noexcept {
    return internal::PrepareIO([=](io_uring_sqe* sqe) { io_uring_prep_write(sqe, fd, buf, nbytes, offset); });
}

} // namespace ak



