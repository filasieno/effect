#pragma once

#include <coroutine>
#include <print>
#include <functional>
#include "dlist.hpp"

struct TaskPromise;
struct SchedulerTaskPromise;
struct SuspendEffect;
struct NopEffect;

typedef std::coroutine_handle<TaskPromise> TaskHdl;
typedef std::coroutine_handle<> CoroHdl;

enum class TaskState {
    READY,      // Ready for execution
    RUNNING,    // Currently running
    IO_WAITING, // Waiting for IO 
    WAITING,    // Waiting for Critical Section
    ZOMBIE      // Already dead
};

struct Task {
    using promise_type = TaskPromise;

    Task() = default;
    
    Task(TaskHdl hdl) : hdl(hdl) {}

    void resume() {
        if (hdl.done()) return;
        hdl.resume();
    }

    void destroy() {
        hdl.destroy();
    }

    bool done() const {
        return hdl.done();
    }

    TaskHdl hdl;
};

struct SchedulerTask {
    using promise_type = TaskPromise;
    
    SchedulerTask(TaskHdl hdl) noexcept : hdl(hdl) {}

    ~SchedulerTask() noexcept {
        hdl.destroy();
    }

    void resume() noexcept {
        hdl.resume();
    }

    bool done() const noexcept {
        return hdl.done();
    }

    TaskState state() const noexcept;

    TaskHdl hdl;
};

struct TaskPromise {
    using Suspend = std::suspend_always;

    struct FinalSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {
            assert(false);
        }
    };

    TaskState state;
    DList     wait_node; // Used to enqueue tasks waiting for Critical Section
    DList     task_list; // Global Task list

    void* operator new(std::size_t n) noexcept;
    void  operator delete(void* ptr, std::size_t sz);

    TaskPromise();
    ~TaskPromise();
    
    TaskHdl get_return_object() noexcept;


    Suspend      initial_suspend() noexcept { return {}; }
    FinalSuspend final_suspend() noexcept { return {}; }
    void         return_void() noexcept;
    void         unhandled_exception() noexcept { assert(false); }
};

inline TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

auto scheduler(std::function<Task()> user_main_task) noexcept -> SchedulerTask;

struct Kernel {
    int task_count;
    int ready_count;      // number of live tasks 
    int waiting_count;    // task waiting for execution (On Internal Critical sections)
    int io_waiting_count; // task waiting for IO URing
    int zombie_count;     // dead tasks

    int          interrupted;
    TaskPromise* current_task_ctx;
    TaskHdl      scheduler_task_hdl;
    DList        zombie_list;
    DList        ready_list;

    DList        task_list;  // global task list
    
    void init() noexcept;
    void fini() noexcept; 


    int run(std::function<Task()> user_main_task) noexcept {
        init();
        
        SchedulerTask scheduler_task = scheduler(user_main_task);
        scheduler_task_hdl = scheduler_task.hdl;

        // Check expected state post scheduler construction
        assert(current_task_ctx == nullptr);
        assert(task_count == 1);
        assert(ready_count == 1);
        assert(scheduler_task.state() == TaskState::READY);
        assert(!scheduler_task.hdl.done());

        // bootstrap task system
        current_task_ctx = &scheduler_task.hdl.promise();
        current_task_ctx->state = TaskState::RUNNING;
        current_task_ctx->wait_node.detach();
        --ready_count;
        
        // Check expected state post task system bootstrap
        check_task_count_invariant();
        
        // Initialize here the scheduler task state 
		scheduler_task.resume(); 
        
        // The scheduler task never returns; it runs until completion
        assert(scheduler_task.hdl.done());
        fini();

        return 0;
    }

    void check_task_count_invariant() {
        assert(task_count - 1 == ready_count + waiting_count + io_waiting_count + zombie_count);  // -1 for the running task
    }

    static SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept {
        std::printf("SchedulerTask: started\n");
        co_return;
    }

} g_kernel;


// -----------------------------------------------------------------------------
// Effects
// -----------------------------------------------------------------------------

struct SuspendEffect {
    
    constexpr bool           await_ready() const noexcept;
    constexpr TaskHdl await_suspend(TaskHdl hdl) const noexcept;
    constexpr void           await_resume() const noexcept;

};

constexpr auto suspend() noexcept-> SuspendEffect { return {}; }


// ==============================================================================
// Implementation 
// ==============================================================================

inline void* TaskPromise::operator new(std::size_t n) noexcept {
    printf("TaskPromise: About to allocate a Task of size: %zu\n", n);
    void* mem = std::malloc(n);
    if (!mem)
        return nullptr;
    unsigned long long frame_head = (unsigned long long)mem;
    unsigned long long frame_tail = frame_head + n;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    printf("TaskPromise: Did allocate a Task(%p) of size: %zu\n", task_addr, n); 
    return mem;
}

inline void TaskPromise::operator delete(void* ptr, std::size_t sz) {
    unsigned long long frame_head = (unsigned long long)ptr;
    unsigned long long frame_tail = frame_head + sz;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    printf("TaskPromise: About to free Task(%p) of size: %zu\n", task_addr, sz);  
    
    std::free(ptr);
    printf("TaskPromise: Did free Task(%p) of size: %zu\n", task_addr, sz);   
}

inline TaskPromise::TaskPromise() {
    ++g_kernel.task_count;    
    task_list.init();
    assert(task_list.detached());
    g_kernel.task_list.push_back(&task_list);

    ++g_kernel.ready_count;
    wait_node.init();
    assert(wait_node.detached());
    g_kernel.ready_list.push_back(&wait_node);

    std::printf("Task(%p): initialized\n", this);
}

inline TaskPromise::~TaskPromise() {
    assert(state == TaskState::ZOMBIE);
    task_list.detach();
    wait_node.detach();
    --g_kernel.task_count;
    std::printf("Task(%p): finalized\n", this);
}

inline TaskHdl TaskPromise::get_return_object() noexcept { 
    std::printf("Task(%p): returning public Handle\n", this);
    return TaskHdl::from_promise(*this);
}

inline void TaskPromise::return_void() noexcept {
    std::printf("Task(%p): has returned\n", this);
    
    g_kernel.check_task_count_invariant();

}

inline constexpr bool SuspendEffect::await_ready() const noexcept { 
    std::printf("Suspend effect: is_ready? (always false)\n");
    return false; 
}

inline constexpr TaskHdl SuspendEffect::await_suspend(TaskHdl hdl) const noexcept {
    // 
    return hdl;

    // TODO : ....

    // TaskContext& promise = hdl.promise();
    // std::printf("Suspend effect: suspending Coro(%p)\n", &promise);
    
    // // Assumptions: 
    // // - the current task `hdl` is stored the `current_task`
    // // - the current task is in RUNNING state
    // // - the current task has an empty wait_list    
    // assert(g_kernel.current_task == &promise);
    // assert(promise.state == TaskState::RUNNING);
    // assert(promise.wait_list.empty());
    // promise.state = TaskState::READY;
    // ++g_kernel.ready_count;
    // g_kernel.ready_list.push_back(&promise.wait_list);
}

inline constexpr void SuspendEffect::await_resume() const noexcept {
    std::printf("Suspend effect: resume\n");
}



void Kernel::init() noexcept {
    task_count = 0;
    ready_count = 0;
    waiting_count = 0;
    io_waiting_count = 0;
    zombie_count = 0;
    interrupted = 0;
    current_task_ctx = nullptr;
    zombie_list.init();
    ready_list.init();
    task_list.init();
    std::printf("Kernel initialized\n");
}

void Kernel::fini() noexcept {
    // TODO: add checks to ensure all tasks are finalized
    std::printf("Kernel finalized\n");
}

static inline TaskPromise* wait_node_to_promise(DList* node) {
    unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, wait_node);
    return (TaskPromise*)promise_off;
}

constexpr TaskHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl) const noexcept {
    

    TaskPromise& current_promise = hdl.promise();                            
    std::printf("FinalSuspend: suspending Task(%p)\n", &current_promise);
    assert(current_promise.state == TaskState::RUNNING);
    assert(g_kernel.current_task_ctx == &current_promise);
    assert(current_promise.wait_node.detached());
    g_kernel.check_task_count_invariant();

    std::printf("Scheduling next task ... \n");
    if (g_kernel.ready_count > 0) {
        std::printf("ready_count > 0\n");
        // Remove current ready node
        current_promise.state = TaskState::ZOMBIE;
        ++g_kernel.zombie_count;
        g_kernel.zombie_list.push_back(&current_promise.wait_node);
        g_kernel.current_task_ctx = nullptr;
        
        // If there are ready tasks, we can just return the current handle
        DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
        TaskPromise* next_ready_promise = wait_node_to_promise(next_ready_promise_node);
        assert(next_ready_promise->state == TaskState::READY);
        next_ready_promise->wait_node.detach();
        --g_kernel.ready_count;
        g_kernel.current_task_ctx = next_ready_promise;
        next_ready_promise->state = TaskState::RUNNING;
        return TaskHdl::from_promise(*next_ready_promise);
    } 
    std::printf("ready_count == 0\n"); 
    
    if (g_kernel.io_waiting_count > 0) {
        std::printf("io_waiting_count > 0\n"); 
        std::printf("unimplemented\n");
        assert(false);
    } 
    std::printf("io_waiting_count == 0\n");  
    // There are no IO waiting tasks
    
    if (g_kernel.waiting_count > 0) {
        std::printf("waiting_count > 0\n");  
        std::printf("deadlock detected\n");
    }
    std::printf("waiting_count == 0\n");   
    // There are no waiting tasks
    g_kernel.interrupted = 1;
    // std::printf("Scheduler {address: %p; promise: %p}\n", g_kernel.scheduler_task_hdl.address(), &g_kernel.scheduler_task_hdl.promise());
    return g_kernel.scheduler_task_hdl;
}