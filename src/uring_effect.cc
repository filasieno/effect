z#include "liburing.h"
#include <coroutine>
#include <unordered_map>
#include <memory>
#include <fcntl.h>

// Task identifier
using PID = uint64_t;

// Represents an async operation result
struct IOResult {
    int result;
    void* data;
};

// Base coroutine promise type
struct TaskPromise {
    struct FinalSuspend {
        bool await_ready() noexcept { return false; }
        void await_resume() noexcept {}
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
            return std::noop_coroutine();
        }
    };

    Task get_return_object();
    std::suspend_always initial_suspend() { return {}; }
    FinalSuspend final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};

// Task coroutine type
struct Task {
    struct promise_type : TaskPromise {
        Task get_return_object() { 
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; 
        }
    };

    std::coroutine_handle<promise_type> handle;
    PID pid;

    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() {
        if (handle) handle.destroy();
    }
};

// Awaitable for io_uring operations
template<typename T>
struct UringAwaitable {
    io_uring* ring;
    io_uring_sqe* sqe;
    IOResult result;

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        // Prepare and submit io_uring operation
        io_uring_sqe_set_data(sqe, h.address());
        io_uring_submit(ring);
    }
    T await_resume() { return result; }
};

// Scheduler/Reactor class
class Reactor {
    io_uring ring;
    std::unordered_map<PID, std::unique_ptr<Task>> tasks;
    PID next_pid = 1;

public:
    Reactor() {
        io_uring_queue_init(256, &ring, 0);
    }

    ~Reactor() {
        io_uring_queue_exit(&ring);
    }

    PID spawn_task(Task task) {
        PID pid = next_pid++;
        task.pid = pid;
        tasks[pid] = std::make_unique<Task>(std::move(task));
        return pid;
    }

    // Example of file number processor coroutine
    static Task process_number(const char* filename) {
        int fd;
        int number;
        
        // Open file
        {
            auto open_op = UringAwaitable<IOResult>{};
            // Setup open operation
            co_await open_op;
            fd = open_op.result.result;
        }

        // Read number
        {
            auto read_op = UringAwaitable<IOResult>{};
            // Setup read operation
            co_await read_op;
            number = *static_cast<int*>(read_op.result.data);
        }

        // Modify number
        number *= 2;

        // Write number
        {
            auto write_op = UringAwaitable<IOResult>{};
            // Setup write operation
            co_await write_op;
        }

        // Close file
        {
            auto close_op = UringAwaitable<IOResult>{};
            // Setup close operation
            co_await close_op;
        }
    }

    void run() {
        while (!tasks.empty()) {
            io_uring_cqe* cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);
            
            if (ret < 0) continue;

            // Get coroutine handle from completion
            auto handle = std::coroutine_handle<>::from_address(
                reinterpret_cast<void*>(io_uring_cqe_get_data(cqe))
            );

            // Process completion result
            // ... set result in awaitable

            // Resume coroutine
            handle.resume();

            io_uring_cqe_seen(&ring, cqe);

            // Clean up completed tasks
            for (auto it = tasks.begin(); it != tasks.end();) {
                if (it->second->handle.done()) {
                    it = tasks.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
};

int main() {
    Reactor reactor;
    
    // Spawn some tasks
    reactor.spawn_task(Reactor::process_number("number.txt"));
    
    // Run the reactor
    reactor.run();
    
    return 0;
}