#include "liburing.h"
#include <coroutine>
#include <print>
#include <errno.h>
#include <sys/mman.h>
#include <functional>
#include "io_utils.h"

typedef unsigned char Byte; 
typedef __u32  U32;  
typedef __u64  U64;   

struct IOSystem {
    int uring_fd;
    
    void* uring_buff;
    int uring_buff_size;    

    void* sqe_buff;
    int   sqe_buff_size;    

    U32* sring_head; 
    U32* sring_tail;
    U32* sring_mask;
    U32* sring_entries;
    U32* sring_flags;
    U32* sring_array;
    
    U32* cring_head;
    U32* cring_tail;
    U32* cring_mask;    
    U32* cring_entries;

    struct io_uring_sqe* sqes;
    struct io_uring_cqe* cqes; 

    auto open_file(const char* path, int flags, mode_t mode, void* user_data = nullptr) noexcept -> int {
        U32 mask = *this->sring_mask;
        U32 tail = *this->sring_tail;
        U32 index = tail & mask; // find the next available SQE

        // Select and fill the next SQE
        struct io_uring_sqe *sqe = &this->sqes[index];
        sqe->opcode = IORING_OP_OPENAT;  
        sqe->fd = AT_FDCWD;              
        sqe->addr = (unsigned long long) path;
        sqe->len = mode;                 
        sqe->open_flags = flags;         
        sqe->user_data = (unsigned long long) user_data;

        this->sring_array[index] = index;

        io_uring_smp_store_release(this->sring_tail, tail + 1);
        
        int ret = io_uring_enter(this->uring_fd, 1, 0, 0, nullptr); 
        if(ret < 0) {
            perror("io_uring_enter");
            return -1;
        }

        return 0;
    } 

    auto read_file(int fd, const void* buff, U64 buff_len, U64 offset, void* user_data = nullptr) noexcept -> int {
        U32 mask = *this->sring_mask;
        U32 tail = *this->sring_tail; 
        U32 index = tail & mask;    
        struct io_uring_sqe *sqe = &this->sqes[index];

        // Fill in the parameters required for the read or write operation 
        sqe->opcode = IORING_OP_READ;
        sqe->fd = fd;
        sqe->addr = (unsigned long long) buff;
        sqe->len = buff_len;
        sqe->off = offset;
        sqe->user_data = (unsigned long long) user_data;

        this->sring_array[index] = index;

        // Update the tail 
        io_uring_smp_store_release(this->sring_tail, tail + 1);

        int ret =  io_uring_enter(this->uring_fd, 1, 0, 0, nullptr); 
        if(ret < 0) {
            perror("io_uring_enter");
            return -1;
        }

        return 0;
    } 

    auto write_file(int fd, const void* buff, U64 buff_len, U64 offset, void* user_data = nullptr) noexcept -> int {
        U32 mask = *this->sring_mask;
        U32 tail = *this->sring_tail;
        U32 index = tail & mask; // find the next available SQE

        // Select and fill the next SQE
        struct io_uring_sqe *sqe = &this->sqes[index];
        sqe->opcode = IORING_OP_WRITE;
        sqe->fd = fd;
        sqe->addr = (unsigned long) buff;
        sqe->len = buff_len;
        sqe->off = offset;

        this->sring_array[index] = index;
        io_uring_smp_store_release(this->sring_tail, tail + 1);
        
        int ret =  io_uring_enter(this->uring_fd, 1, 0, 0, nullptr); 
        if(ret < 0) {
            perror("io_uring_enter");
            return -1;
        }

        return 0;
    } 

    auto close(int fd, void* user_data = nullptr) noexcept -> int {
        U32 mask = *this->sring_mask;
        U32 tail = *this->sring_tail; 
        U32 index = tail & mask;    
        struct io_uring_sqe *sqe = &this->sqes[index];

        // Fill in the parameters required for the read or write operation 
        sqe->opcode = IORING_OP_CLOSE;
        sqe->fd = fd;
        sqe->user_data = (unsigned long) user_data;

        this->sring_array[index] = index;
        io_uring_smp_store_release(this->sring_tail, tail + 1);
        
        int ret =  io_uring_enter(this->uring_fd, 1, 0, 0, nullptr); 

        if(ret < 0) {
            perror("io_uring_enter");
            return -1;
        }
        
        return 0;
    } 

    auto init(struct io_uring_params* params, unsigned int entries = 1024) noexcept -> int { 
        
        // params->flags = IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
        this->uring_fd = io_uring_setup(entries, params);
        if (this->uring_fd < 0) {
            std::printf("io_uring_setup failed; error: %zu\n", -this->uring_fd);
            return -1;
        }
        io_utils::uring_debug_params(*params);
        std::printf("io_uring_setup done\n");

        // mmap SQ and CQ
        if (!(params->features & IORING_FEAT_SINGLE_MMAP)) {
            std::printf("IORING_FEAT_SINGLE_MMAP not supported\n");
            ::close(this->uring_fd);
            return -1;
        }

        // io_uring communication happens via 2 shared kernel-user space ring
        // buffers, which can be jointly mapped with a single mmap() call in
        // kernels >= 5.4.
        //
        int sring_sz = params->sq_off.array + params->sq_entries * sizeof(unsigned);
        int cring_sz = params->cq_off.cqes  + params->cq_entries * sizeof(struct io_uring_cqe);
        this->uring_buff_size = std::max(sring_sz, cring_sz);

        // Setup of: sring + cring + cqes array
        this->uring_buff = mmap(0, this->uring_buff_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, this->uring_fd, IORING_OFF_SQ_RING); 
        if (this->uring_buff == MAP_FAILED) {
            perror("mmap");
            ::close(this->uring_fd);
            return -1;
        }

        this->sring_head    = (U32*)offset_from(this->uring_buff, params->sq_off.head);
        this->sring_tail    = (U32*)offset_from(this->uring_buff, params->sq_off.tail);
        this->sring_mask    = (U32*)offset_from(this->uring_buff, params->sq_off.ring_mask);
        this->sring_entries = (U32*)offset_from(this->uring_buff, params->sq_off.ring_entries);
        this->sring_flags   = (U32*)offset_from(this->uring_buff, params->sq_off.flags);
        this->sring_array   = (U32*)offset_from(this->uring_buff, params->sq_off.array);
        this->cring_head    = (U32*)offset_from(this->uring_buff, params->cq_off.head);
        this->cring_tail    = (U32*)offset_from(this->uring_buff, params->cq_off.tail);
        this->cring_mask    = (U32*)offset_from(this->uring_buff, params->cq_off.ring_mask);
        this->cring_entries = (U32*)offset_from(this->uring_buff, params->cq_off.ring_entries);
        this->cqes = (struct io_uring_cqe*)offset_from(this->uring_buff, params->cq_off.cqes);     

        // Setup of: sqe array 
        this->sqe_buff_size = params->sq_entries * sizeof(struct io_uring_sqe);
        this->sqe_buff = (struct io_uring_sqe*)mmap(0, this->sqe_buff_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, this->uring_fd, IORING_OFF_SQES); 

        if (this->sqe_buff == MAP_FAILED) {
            perror("mmap io->sqes");
            munmap(this->uring_buff, this->uring_buff_size);
            ::close(this->uring_fd);
            return -1;
        }
        this->sqes = (struct io_uring_sqe*)this->sqe_buff;
        std::printf("CQ initialized\n");
        std::printf("IO ring initialized\n");
        return 0;
    }

    auto fini() noexcept -> void {
        munmap(this->uring_buff, this->uring_buff_size); 
        munmap(this->sqe_buff, this->sqe_buff_size);
        ::close(this->uring_fd);
    }

    auto wait_one(struct io_uring_cqe** opt_out = nullptr) noexcept -> int {   
        // Check if we have any completions
        U32 head = *this->cring_head;
        U32 tail = *this->cring_tail;
        std::printf("Waiting for one completion: head=%u, tail=%u\n", head, tail);
        if (head == tail) {
            std::printf("no cqe available, waiting...\n");
            int ret = io_uring_enter(this->uring_fd, 0, 1, IORING_ENTER_GETEVENTS, nullptr);
            if (ret < 0) {
                std::printf("io_uring_enter failed\n");
                return -1; 
            }
            std::printf("waiting done -> one cqe delivered\n");
        }

        U32 mask = *this->cring_mask;
        struct io_uring_cqe* cqe = &this->cqes[head & mask];
        
        // If caller wants the CQE data, copy it
        if (opt_out) {
            *opt_out = cqe;
        }

        // Advance the head
        io_uring_smp_store_release(this->cring_head, head + 1);

        return 0; 
    }
    
};

IOSystem io;



// Completion range class to support range-based for loops
class CompletionRange {
    struct Iterator {
        IOSystem* io;
        unsigned index;
        unsigned tail;
        
        Iterator(IOSystem* io, unsigned index, unsigned tail) 
            : io(io), index(index), tail(tail) {}
        
        bool operator!=(const Iterator& other) const {
            return index != other.index;
        }
        
        Iterator& operator++() {
            // Release the completion slot immediately after use
            io_uring_smp_store_release(io->cring_head, index + 1);
            index++;
            return *this;
        }
        
        struct io_uring_cqe& operator*() {
            return io->cqes[index & *io->cring_mask];
        }
    };
    
    IOSystem* io;
    unsigned head;
    unsigned tail;
    
public:
    CompletionRange(IOSystem* io) 
        : io(io)
        , head(*io->cring_head)
        , tail(*io->cring_tail) 
    {
        std::printf("Completion range created with head: %u, tail: %u\n", head, tail);            
    }
    
    Iterator begin() { return Iterator(io, head, tail); }
    Iterator end() { return Iterator(io, tail, tail); }
    
    // No need for final release in destructor since we release per iteration
    ~CompletionRange() = default;
};

CompletionRange io_get_completions(IOSystem* io) {
    return CompletionRange(io);
}

void debug_cqe(const struct io_uring_cqe& cqe, const IOSystem* io) {
    std::printf("\nCQE Debug Info:\n");
    std::printf("  Result: %d\n", cqe.res);
    
    // First handle errors
    if (cqe.res < 0) {
        switch (-cqe.res) {
            case EBADF:  std::printf("  Error: Bad file descriptor\n"); break;
            case EINVAL: std::printf("  Error: Invalid argument\n"); break;
            case EACCES: std::printf("  Error: Permission denied\n"); break;
            case ENOENT: std::printf("  Error: No such file or directory\n"); break;
            case EAGAIN: std::printf("  Error: Resource temporarily unavailable\n"); break;
            default:     std::printf("  Error code: %d\n", -cqe.res); break;
        }
        std::printf("-------------------\n");
        return;
    }

    // Get the submission queue state
    U32 head = *io->sring_head;
    U32 tail = *io->sring_tail;
    U32 mask = *io->sring_mask;
    
    // Find the corresponding SQE index using the ring array
    U32 sqe_index = io->sring_array[head & mask];
    
    // Validate the index
    if (sqe_index > mask) {
        std::printf("  Error: Invalid SQE index\n");
        std::printf("-------------------\n");
        return;
    }

    // Get the SQE
    const struct io_uring_sqe* sqe = &io->sqes[sqe_index];
    
    // Print operation info based on opcode
    std::printf("  SQE Index: %u\n", sqe_index);
    std::printf("  Operation: ");
    
    switch (sqe->opcode) {
        case IORING_OP_OPENAT:
            std::printf("OPEN\n");
            std::printf("  Path: %s\n", (const char*)sqe->addr);
            std::printf("  Flags: 0x%x\n", sqe->open_flags);
            std::printf("  Mode: 0%o\n", sqe->len);
            if (cqe.res >= 0) {
                std::printf("  Assigned fd: %d\n", cqe.res);
            }
            break;
            
        case IORING_OP_READ:
            std::printf("READ\n");
            std::printf("  fd: %d\n", sqe->fd);
            std::printf("  Buffer: %p\n", (void*)sqe->addr);
            std::printf("  Length: %u\n", sqe->len);
            std::printf("  Offset: %llu\n", sqe->off);
            if (cqe.res >= 0) {
                std::printf("  Bytes read: %d\n", cqe.res);
            }
            break;
            
        case IORING_OP_WRITE:
            std::printf("WRITE\n");
            std::printf("  fd: %d\n", sqe->fd);
            std::printf("  Buffer: %p\n", (void*)sqe->addr);
            std::printf("  Length: %u\n", sqe->len);
            std::printf("  Offset: %llu\n", sqe->off);
            if (cqe.res >= 0) {
                std::printf("  Bytes written: %d\n", cqe.res);
            }
            break;
            
        case IORING_OP_CLOSE:
            std::printf("CLOSE\n");
            std::printf("  fd: %d\n", sqe->fd);
            if (cqe.res == 0) {
                std::printf("  Status: Successfully closed\n");
            }
            break;
            
        default:
            std::printf("UNKNOWN (%d)\n", sqe->opcode);
    }

    // Print CQE flags if any
    if (cqe.flags) {
        std::printf("  CQE Flags: 0x%x\n", cqe.flags);
        if (cqe.flags & IORING_CQE_F_BUFFER) {
            std::printf("    - Buffer selected\n");
        }
        if (cqe.flags & IORING_CQE_F_MORE) {
            std::printf("    - More data available\n");
        }
        if (cqe.flags & IORING_CQE_F_SOCK_NONEMPTY) {
            std::printf("    - Socket non-empty\n");
        }
    }

    // Print SQE flags if any
    if (sqe->flags) {
        std::printf("  SQE Flags: 0x%x\n", sqe->flags);
        if (sqe->flags & IOSQE_FIXED_FILE) {
            std::printf("    - Fixed file\n");
        }
        if (sqe->flags & IOSQE_IO_DRAIN) {
            std::printf("    - IO drain\n");
        }
        if (sqe->flags & IOSQE_IO_LINK) {
            std::printf("    - IO link\n");
        }
        if (sqe->flags & IOSQE_IO_HARDLINK) {
            std::printf("    - IO hardlink\n");
        }
        if (sqe->flags & IOSQE_ASYNC) {
            std::printf("    - Async\n");
        }
    }

    if (sqe->user_data) {
        std::printf("  User data: 0x%llx\n", (unsigned long long)sqe->user_data);
    }
    
    std::printf("-------------------\n");
}

int main() {
    struct io_uring_params params = {};
    if (io.init(&params) != 0) return -1;
    int res;
    struct io_uring_cqe* cqe;    

    // open
    res = io.open_file("test.txt", O_RDWR | O_CREAT | O_TRUNC | O_NONBLOCK, 0666);  
    if (res < 0) {
        std::printf("Failed to submit file open operation\n");
        io.fini(); 
        return -1;
    }
    std::printf("open file operation submitted\n");

    // wait
    res = io.wait_one(&cqe);
    
    if (res < 0) {
        std::printf("Failed to wait for completion\n");
        io.fini();
        return -1;
    }
    
    // write
    int fd = cqe->res; // Get the file descriptor from the completion event
    res = io.write_file(fd, "Hello World!", 12, 0);
    if (res < 0) {
        std::printf("Failed to submit file write operation\n");
        io.fini();
        return -1;
    }
    std::printf("write file operation submitted\n");

    // wait
    res = io.wait_one(&cqe);
    if (res < 0) {
        std::printf("Failed to wait for completion\n");
        io.fini();
        return -1;
    }

    // close
    res = io.close(fd);
    if (res < 0) {
        std::printf("Failed to submit file close operation\n");
        io.fini();
        return -1;
    }
    std::printf("close file operation submitted\n");

    res = io.wait_one(&cqe);
    if (res < 0) {
        std::printf("Failed to wait for completion\n");
        io.fini();
        return -1;
    }

    io.fini();
    return 0;    
}