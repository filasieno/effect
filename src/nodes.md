# IO Ring notes

## `io_uring_params`

*sq_entries*: Submission queue size (number of entries). You specify your desired size, and the kernel may adjust it.

*cq_entries*: Completion queue size. If left as 0, the kernel will set it to double of sq_entries.

*flags*: Configuration flags for io_uring setup. Common flags include:
 - IORING_SETUP_SQPOLL: Enable kernel-side polling
 - IORING_SETUP_IOPOLL: Enable IO polling
 - IORING_SETUP_SQ_AFF: Set CPU affinity for SQ thread

*sq_thread_cpu*: If IORING_SETUP_SQPOLL is set, this specifies which CPU the kernel thread should run on. 

*sq_thread_idle*: The idle timeout for the kernel's submission queue polling thread (in milliseconds).

*features*: Returns supported features from the kernel. Read-only field set by the kernel.

*wq_fd*: File descriptor for sharing kernel worker thread pool between multiple rings.

*sq_off*: Structure containing offsets for various submission queue fields:
- head
- tail
- ring mask
- ring entries
- flags
- dropped
- array

*cq_off*: Structure containing offsets for various completion queue fields:
- head
- tail
- ring mask
- ring entries
- overflow
- cqes
- flags

