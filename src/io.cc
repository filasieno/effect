#include "liburing.h"
#include <coroutine>
#include <print>
#include <errno.h>
#include <sys/mman.h>
#include <functional>

typedef unsigned char Byte; 
typedef __u32  U32;  
typedef __u64  U64;   

struct IOSystem {
    int uring_fd;
    void* uring_buff;
    int uring_buff_size;    

    U32* sring_tail;
    U32* sring_mask;
    U32* sring_array;
    
    U32* cring_head;
    U32* cring_tail;
    U32* cring_mask; 

    struct io_uring_sqe* sqes;
    struct io_uring_cqe* cqes; 
};

IOSystem io;

void print_io_uring_features(const unsigned int features) {
    std::printf("Features:\n");
    if (features & IORING_FEAT_SINGLE_MMAP) std::printf("  SINGLE_MMAP\n");
    if (features & IORING_FEAT_NODROP) std::printf("  NODROP\n");
    if (features & IORING_FEAT_SUBMIT_STABLE) std::printf("  SUBMIT_STABLE\n");
    if (features & IORING_FEAT_RW_CUR_POS) std::printf("  RW_CUR_POS\n");
    if (features & IORING_FEAT_CUR_PERSONALITY) std::printf("  CUR_PERSONALITY\n");
    if (features & IORING_FEAT_FAST_POLL) std::printf("  FAST_POLL\n");
    if (features & IORING_FEAT_POLL_32BITS) std::printf("  POLL_32BITS\n");
    if (features & IORING_FEAT_SQPOLL_NONFIXED) std::printf("  SQPOLL_NONFIXED\n");
    if (features & IORING_FEAT_EXT_ARG) std::printf("  EXT_ARG\n");
    if (features & IORING_FEAT_NATIVE_WORKERS) std::printf("  NATIVE_WORKERS\n");
}

void print_io_uring_flags(const unsigned int flags) {
    std::printf("Flags:\n");
    if (flags & IORING_SETUP_IOPOLL) std::printf("  IOPOLL\n");
    if (flags & IORING_SETUP_SQPOLL) std::printf("  SQPOLL\n");
    if (flags & IORING_SETUP_SQ_AFF) std::printf("  SQ_AFF\n");
    if (flags & IORING_SETUP_CQSIZE) std::printf("  CQSIZE\n");
    if (flags & IORING_SETUP_CLAMP) std::printf("  CLAMP\n");
    if (flags & IORING_SETUP_ATTACH_WQ) std::printf("  ATTACH_WQ\n");
}

void print_io_uring_params(const struct io_uring_params& p) {
    std::printf("\n=== IO_URING PARAMETERS ===\n\n");
    
    // Main parameters
    std::printf("Main Configuration:\n");
    std::printf("  sq_entries: %u\n", p.sq_entries);
    std::printf("  cq_entries: %u\n", p.cq_entries);
    std::printf("  sq_thread_cpu: %u\n", p.sq_thread_cpu);
    std::printf("  sq_thread_idle: %u\n", p.sq_thread_idle);
    std::printf("  wq_fd: %u\n", p.wq_fd);

    // Print flags
    print_io_uring_flags(p.flags);

    // Print features
    print_io_uring_features(p.features);

    // Submission Queue Offsets

    std::printf("Submission Queue Offsets:\n");
    std::printf("  head: %u\n", p.sq_off.head);
    std::printf("  tail: %u\n", p.sq_off.tail);
    std::printf("  ring_mask: %u\n", p.sq_off.ring_mask);
    std::printf("  ring_entries: %u\n", p.sq_off.ring_entries);
    std::printf("  flags: %u\n", p.sq_off.flags);
    std::printf("  dropped: %u\n", p.sq_off.dropped);
    std::printf("  array: %u\n", p.sq_off.array);

    // Completion Queue Offsets
    std::printf("Completion Queue Offsets:\n");
    std::printf("  head: %u\n", p.cq_off.head);
    std::printf("  tail: %u\n", p.cq_off.tail);
    std::printf("  ring_mask: %u\n", p.cq_off.ring_mask);
    std::printf("  ring_entries: %u\n", p.cq_off.ring_entries);
    std::printf("  overflow: %u\n", p.cq_off.overflow);
    std::printf("  cqes: %u\n", p.cq_off.cqes);
    std::printf("  flags: %u\n", p.cq_off.flags);
    std::printf("\n");

    std::printf("================================\n");
}

inline void* offset_from(void* base, unsigned int offset) { 
    return (void*)((Byte*)base + offset);
}

int io_init(IOSystem* io, struct io_uring_params* params, unsigned int entries = 1024) { 
    
    params->flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
    io->uring_fd = io_uring_setup(entries, params);
    if (io->uring_fd < 0) {
        std::printf("io_uring_setup failed; error: %zu\n", -io->uring_fd);
        return -1;
    }
    print_io_uring_params(*params);
    std::printf("io_uring_setup done\n");

    // mmap SQ and CQ
    if (!(params->features & IORING_FEAT_SINGLE_MMAP)) {
        std::printf("IORING_FEAT_SINGLE_MMAP not supported\n");
        close(io->uring_fd);
        return -1;
    }

    // io_uring communication happens via 2 shared kernel-user space ring
    // buffers, which can be jointly mapped with a single mmap() call in
    // kernels >= 5.4.
    //
    int sring_sz = params->sq_off.array + params->sq_entries * sizeof(unsigned);
    int cring_sz = params->cq_off.cqes  + params->cq_entries * sizeof(struct io_uring_cqe);
    io->uring_buff_size = std::max(sring_sz, cring_sz);

    // mmap the rings
    io->uring_buff = mmap(0, io->uring_buff_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, io->uring_fd, IORING_OFF_SQ_RING);
    if (io->uring_buff == MAP_FAILED) {
        perror("mmap");
        close(io->uring_fd);
        return -1;
    }
    io->sring_tail  = (U32*)offset_from(io->uring_buff, params->sq_off.tail);
    io->sring_mask  = (U32*)offset_from(io->uring_buff, params->sq_off.ring_mask); 
    io->sring_array = (U32*)offset_from(io->uring_buff, params->sq_off.array);  
    io->cring_head  = (U32*)offset_from(io->uring_buff, params->cq_off.head);  
    io->cring_tail  = (U32*)offset_from(io->uring_buff, params->cq_off.tail);
    io->cring_mask  = (U32*)offset_from(io->uring_buff, params->cq_off.ring_mask);
    
    io->sqes = (struct io_uring_sqe*)mmap(0, params->sq_entries * sizeof(struct io_uring_sqe), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, io->uring_fd, IORING_OFF_SQES);
    if (io->sqes == MAP_FAILED) {
        perror("mmap io->sqes");
        munmap(io->uring_buff, io->uring_buff_size);
        close(io->uring_fd);
        return -1;
    }
    io->cqes = (struct io_uring_cqe*)offset_from(io->uring_buff, params->cq_off.cqes);
    
    std::printf("CQ initialized\n");
    std::printf("IO ring initialized\n");
    return 0;
}

void io_fini(IOSystem& io) {
    munmap(io.uring_buff, io.uring_buff_size);
    close(io.uring_fd);
}

int io_submit_read(IOSystem* io, int fd, void* buff, U64 buff_len, U64 offset) noexcept {
    U32 index, tail;

    // Add our submission queue entry to the tail of the SQE ring buffer 
    tail = *io->sring_tail;
    index = tail & *io->sring_mask;
    struct io_uring_sqe *sqe = &io->sqes[index];

    // Fill in the parameters required for the read or write operation 
    sqe->opcode = IORING_OP_READ;
    sqe->fd = fd;
    sqe->addr = (unsigned long) buff;
    sqe->len = buff_len;
    sqe->off = offset;

    io->sring_array[index] = index;
    tail++;

    // Update the tail 
    io_uring_smp_store_release(io->sring_tail, tail);

    
    // Tell the kernel we have submitted events with the io_uring_enter()
    // system call. We also pass in the IOURING_ENTER_GETEVENTS flag which
    // causes the io_uring_enter() call to wait until min_complete
    // (the 3rd param) events complete.
    
    int ret =  io_uring_enter(io->uring_fd, 1, 1, IORING_ENTER_GETEVENTS, nullptr); 
    if(ret < 0) {
        perror("io_uring_enter");
        return -1;
    }

    return ret;
}



int main() {
    struct io_uring_params params = {};
    if (io_init(&io, &params) != 0) return -1;

    io_fini(&io);
    return 0;    
}