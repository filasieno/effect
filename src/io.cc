#include "liburing.h"
#include <coroutine>
#include <print>
#include <errno.h>
#include <sys/mman.h>

struct IOSystem {
    int uring_fd;
    struct io_uring_sqe* sqe;
    struct io_uring_cqe* cqe;
};

IOSystem io;

int main() {
    // setup io_uring
    constexpr unsigned int entries = 512;
    struct io_uring_params params = {0};
    params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
    io.uring_fd = io_uring_setup(entries, &params);
    if (io.uring_fd < 0) {
        std::printf("io_uring_setup failed; error: %zu\n", -io.uring_fd);
        return -1;
    }
    std::printf("io_uring_setup done\n");

    // mmap SQ and CQ

    // io_uring communication happens via 2 shared kernel-user space ring
    // buffers, which can be jointly mapped with a single mmap() call in
    // kernels >= 5.4.
    //
    // int sring_sz = params.sq_off.array + params.sq_entries * sizeof(unsigned);
    // int cring_sz = params.cq_off.cqes  + params.cq_entries * sizeof(struct io_uring_cqe);
    // if (params.features & IORING_FEAT_SINGLE_MMAP) {
    //     if (cring_sz > sring_sz) sring_sz = cring_sz;
    //     cring_sz = sring_sz;
    // }

    
    // int ring_sz = sring_sz + cring_sz;

    // // mmap the rings
    //  sq_ptr = (struct io_urin_sqe*)mmap(0, sring_sz, PROT_READ | PROT_WRITE,
    //                      MAP_SHARED | MAP_POPULATE,
    //                      ring_fd, IORING_OFF_SQ_RING);
    //        if (sq_ptr == MAP_FAILED) {
    //            perror("mmap");
    //            return 1;
    //        }

    
    if (close(io.uring_fd) != 0) {
        std::printf("close failed; error: %zu\n", errno);
    } else {
        std::printf("io_uring closed\n"); 
    }
    return 0;
}