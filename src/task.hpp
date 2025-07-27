#pragma once

#include <coroutine>
#include <print>
#include <functional>
#include "dlist.hpp"

struct TaskPromise;
struct SchedulerTaskPromise;
struct SuspendEffect;
struct NopEffect;

using TaskHdl = std::coroutine_handle<TaskPromise>;
using CoroHdl = std::coroutine_handle<>;

enum class TaskState {
    CREATED,    // Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
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

    void resume() noexcept;

    void destroy() {
        hdl.destroy();
    }

    bool done() const {
        return hdl.done();
    }

    TaskState state() const;

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
    struct FinalSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr CoroHdl await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept {
            std::printf("TaskPromise::FinalSuspend: illegal await_resume\n");
            assert(false);
        }
    };
    
    struct InitialSuspend {
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_suspend(TaskHdl hdl) const noexcept;
        constexpr void await_resume() const noexcept;
    };

    TaskState state;
    DList     wait_node; // Used to enqueue tasks waiting for Critical Section
    DList     task_list; // Global Task list

    void* operator new(std::size_t n) noexcept;
    void  operator delete(void* ptr, std::size_t sz);

    TaskPromise();
    ~TaskPromise();
    
    TaskHdl get_return_object() noexcept;

    InitialSuspend initial_suspend() noexcept { return {}; }
    FinalSuspend   final_suspend() noexcept { return {}; }
    void           return_void() noexcept;
    void           unhandled_exception() noexcept { assert(false); }
};

inline TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

inline TaskState Task::state() const {  
    return hdl.promise().state;
}

struct Kernel {
    int task_count;
    int ready_count;      // number of live tasks 
    int waiting_count;    // task waiting for execution (On Internal Critical sections)
    int io_waiting_count; // task waiting for IO URing
    int zombie_count;     // dead tasks

    int          interrupted;
    TaskPromise* current_task_promise;
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
        assert(current_task_promise == nullptr);
        assert(task_count == 1);
        assert(ready_count == 1);
        assert(scheduler_task.state() == TaskState::READY);
        assert(!scheduler_task.hdl.done());

        // bootstrap task system
        current_task_promise = &scheduler_task.hdl.promise();
        current_task_promise->state = TaskState::RUNNING;
        current_task_promise->wait_node.detach();
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

    void check_task_count_invariant() noexcept;

    static SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept;

} g_kernel;

static CoroHdl schedule_next_task(TaskHdl hdl) noexcept;

static void debug_task_count() noexcept {
    int running_count = g_kernel.current_task_promise != nullptr ? 1 : 0;
    std::printf("Task       count: %d\n", g_kernel.task_count);
    std::printf("----------------:--------\n"); 
    std::printf("Running    count: %d\n", running_count); 
    std::printf("Ready      count: %d\n", g_kernel.ready_count);
    std::printf("Waiting    count: %d\n", g_kernel.waiting_count);
    std::printf("IO waiting count: %d\n", g_kernel.io_waiting_count);
    std::printf("Zombie     count: %d\n", g_kernel.zombie_count);
}

inline constexpr void TaskPromise::InitialSuspend::await_resume() const noexcept {
    std::printf("TaskPromise::InitialSuspend::await_resume(): started Task(%p); state: %d\n", g_kernel.current_task_promise, g_kernel.current_task_promise->state);
}

// -----------------------------------------------------------------------------
// Effects
// -----------------------------------------------------------------------------

struct SuspendEffect {
    
    constexpr bool    await_ready() const noexcept;
    constexpr CoroHdl await_suspend(TaskHdl hdl) const noexcept;
    constexpr void    await_resume() const noexcept;

};

constexpr auto suspend() noexcept-> SuspendEffect { return {}; }
static TaskPromise* wait_node_to_promise(DList* node);

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
    task_list.init();
    wait_node.init();
    state = TaskState::CREATED;
    std::printf("Task(%p): created\n", this);
    
    // Check post-conditions
    assert(task_list.detached());
    assert(wait_node.detached());
    assert(state == TaskState::CREATED);
    g_kernel.check_task_count_invariant();
}

inline TaskPromise::~TaskPromise() {
    assert(state == TaskState::ZOMBIE);
    task_list.detach();
    wait_node.detach();
    std::printf("Task(%p): finalized\n", this);

    // todo: move await from destructor
    --g_kernel.task_count;
    --g_kernel.zombie_count;
    g_kernel.check_task_count_invariant();
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

inline constexpr CoroHdl SuspendEffect::await_suspend(TaskHdl current_task_hdl) const noexcept {
    TaskPromise& current_promise = current_task_hdl.promise();

    // Ensure that the current task is RUNNING and that it is correctly set as the current task
    assert(g_kernel.current_task_promise == &current_promise);
    assert(current_promise.state == TaskState::RUNNING);
    assert(current_promise.wait_node.detached());
    g_kernel.check_task_count_invariant();
    
    std::printf("SuspendEffect: suspending task(%p)\n", g_kernel.current_task_promise);
    std::printf("Task(%p): scheduleNextTask() suspending the current task\n", &current_promise); 
    
    debug_task_count();
    // Case 1: There are ready Tasks 
    if (g_kernel.ready_count > 1) {
        // consider that a scheduler task is always running
        std::printf("Task(%p): scheduleNextTask(): ready count > 1; ready count: %d\n", &current_promise, g_kernel.ready_count); 

        // Remove current ready node
        current_promise.state = TaskState::READY;
        ++g_kernel.ready_count;
        g_kernel.ready_list.push_back(&current_promise.wait_node);
        g_kernel.current_task_promise = nullptr;

        // Check invariants
        g_kernel.check_task_count_invariant();
        
        // Pop the next ready task from the ready list
        DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
        --g_kernel.ready_count;
        TaskPromise* next_ready_promise = wait_node_to_promise(next_ready_promise_node);
        assert(next_ready_promise->state == TaskState::READY);
        next_ready_promise->wait_node.detach();
        g_kernel.current_task_promise = next_ready_promise;
        next_ready_promise->state = TaskState::RUNNING;
        return TaskHdl::from_promise(*next_ready_promise);
    }     
    std::printf("scheduleNextTask(TaskHdl hdl): ready count: 1");
    // not Case 1: There are no ready tasks ...


    // -------------------------------------------------------------------------
    // Case 2: There are IO waiting tasks
    // -------------------------------------------------------------------------
    if (g_kernel.io_waiting_count > 0) {
        std::printf("scheduleNextTask(TaskHdl hdl): io_waiting_count > 0; io_waiting_count: %d\n", g_kernel.io_waiting_count);
        std::printf("unimplemented\n");
        assert(false);
    }     
    std::printf("io_waiting_count == 0\n");  
    // not Case 2:There are no IO waiting tasks
    
    
    // -------------------------------------------------------------------------
    // Case 3: There are waiting task
    // -------------------------------------------------------------------------
    if (g_kernel.waiting_count > 0) {
        std::printf("scheduleNextTask(TaskHdl hdl): io_waiting_count > 0; waiting_count: %d\n", g_kernel.waiting_count);
        std::printf("scheduleNextTask(TaskHdl hdl): deadlock detected\n");
        assert(false);
    }
    std::printf("scheduleNextTask(TaskHdl hdl): waiting_count: 0\n");    
    
    // -------------------------------------------------------------------------
    // Case 3: Its the last task running
    // -------------------------------------------------------------------------

    assert(&g_kernel.scheduler_task_hdl.promise() == g_kernel.current_task_promise);
    std::printf("scheduleNextTask(TaskHdl hdl): The scheduler is the last task running\n");
    g_kernel.interrupted = 1;
    g_kernel.scheduler_task_hdl.promise().state = TaskState::ZOMBIE;
    
    // Noop coroutine will force a symetric transfer exit; use std::coroutine_handle<> as a more flexible return type
    return std::noop_coroutine();
}

inline constexpr void SuspendEffect::await_resume() const noexcept {
    std::printf("SuspendEffect::await_resume(): resumed Task(%p)\n", g_kernel.current_task_promise);
}

void Kernel::init() noexcept {
    task_count = 0;
    ready_count = 0;
    waiting_count = 0;
    io_waiting_count = 0;
    zombie_count = 0;
    interrupted = 0;
    current_task_promise = nullptr;
    zombie_list.init();
    ready_list.init();
    task_list.init();
    std::printf("Kernel::init(): initialized\n");

    // Check invariants
    g_kernel.check_task_count_invariant();
}

void Kernel::fini() noexcept {
    // TODO: add checks to ensure all tasks are finalized

    std::printf("Kernel::fini(): finalized\n");
    
    // Check invariants
    g_kernel.check_task_count_invariant();
}

static inline TaskPromise* wait_node_to_promise(DList* node) {
    unsigned long long promise_off = ((unsigned long long)node) - offsetof(TaskPromise, wait_node);
    return (TaskPromise*)promise_off;
}

constexpr CoroHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& current_promise = hdl.promise();                            
    
    // Check preconditions
    assert(current_promise.state == TaskState::RUNNING);
    assert(g_kernel.current_task_promise == &current_promise);
    assert(current_promise.wait_node.detached());
    g_kernel.check_task_count_invariant();

    
    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): suspending Task(%p)\n", &current_promise);
    if (g_kernel.ready_count > 0) {
        std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): ready count > 0; ready count: %d\n", g_kernel.ready_count);
        // Remove current ready node
        current_promise.state = TaskState::ZOMBIE;
        ++g_kernel.zombie_count;
        g_kernel.zombie_list.push_back(&current_promise.wait_node);
        g_kernel.current_task_promise = nullptr;
        
        // If there are ready tasks, we can just return the current handle
        DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
        TaskPromise* next_ready_promise = wait_node_to_promise(next_ready_promise_node);
        assert(next_ready_promise->state == TaskState::READY);
        next_ready_promise->wait_node.detach();
        --g_kernel.ready_count;
        g_kernel.current_task_promise = next_ready_promise;
        next_ready_promise->state = TaskState::RUNNING;
        return TaskHdl::from_promise(*next_ready_promise);
    } 
    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): ready count: 0");
    
    if (g_kernel.io_waiting_count > 0) {
        std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): io_waiting_count > 0; io_waiting_count: %d\n", g_kernel.io_waiting_count);
        std::printf("unimplemented\n");
        assert(false);
    } 
    std::printf("io_waiting_count == 0\n");  
    // There are no IO waiting tasks
    
    if (g_kernel.waiting_count > 0) {
        std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): io_waiting_count > 0; waiting_count: %d\n", g_kernel.waiting_count);
        std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): deadlock detected\n");
        assert(false);
    }
    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): waiting_count: 0\n");    
    
    // There are no waiting tasks
    
    assert(&g_kernel.scheduler_task_hdl.promise() == g_kernel.current_task_promise);
    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): The scheduer is the last task running\n");
    g_kernel.interrupted = 1;
    g_kernel.scheduler_task_hdl.promise().state = TaskState::ZOMBIE;
    
    // Noop coroutine will force a symetric transfer exit; use std::coroutine_handle<> as a more flexible return type
    return std::noop_coroutine();
}

SchedulerTask Kernel::scheduler(std::function<Task()> user_main_task) noexcept {
    std::printf(">> SchedulerTask(%p): started\n", g_kernel.current_task_promise);
    std::printf(">> SchedulerTask(%p): about to spawn main task\n", g_kernel.current_task_promise); 
    Task main_task = user_main_task();
    assert(!main_task.hdl.done());
    assert(main_task.state() == TaskState::READY);
    std::printf(">> SchedulerTask(%p): did spawn main task\n", g_kernel.current_task_promise);
    main_task.resume();
    co_return;
}

void Task::resume() noexcept {
    // Check invariants
    g_kernel.check_task_count_invariant();
    
    if (hdl.done()) return;
    std::printf("Task(%p)::resume(): requested to resume Task(%p)\n", g_kernel.current_task_promise, &hdl.promise());
    
    // Ensure that the target task is READY
    TaskPromise* target_promise = &hdl.promise();
    assert(target_promise->state == TaskState::READY);
    assert(!target_promise->wait_node.detached());
    // todo: check that the target task is in the ready list

    // Ensure that the current task is RUNNING
    TaskPromise* current_promise = g_kernel.current_task_promise;  
    assert(current_promise != nullptr);
    assert(current_promise->state == TaskState::RUNNING);
    assert(current_promise->wait_node.detached());

    // Move the current task from RUNNING to READY
    current_promise->state = TaskState::READY;
    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&current_promise->wait_node);
    
    // Move the target task from READY to RUNNING
    --g_kernel.ready_count;
    target_promise->wait_node.detach();
    target_promise->state = TaskState::RUNNING;
    g_kernel.current_task_promise = target_promise;

    std::printf("Task(%p)::resume(): is the new RUNNING Task\n", g_kernel.current_task_promise);
    
    // Check post-conditions
    assert(g_kernel.current_task_promise == target_promise);
    assert(g_kernel.current_task_promise->state == TaskState::RUNNING);
    assert(g_kernel.current_task_promise->wait_node.detached());
    g_kernel.check_task_count_invariant();

    // Resume the target task
    hdl.resume();
}

 constexpr void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& promise = hdl.promise();

    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(promise.wait_node.detached());
    g_kernel.check_task_count_invariant();

    // Add task to the kernel
    ++g_kernel.task_count;    
    g_kernel.task_list.push_back(&promise.task_list);

    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&promise.wait_node);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!promise.wait_node.detached()); 
    g_kernel.check_task_count_invariant();

    std::printf("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task(%p) is READY\n", &promise, promise.state); 
}

void Kernel::check_task_count_invariant() noexcept {
    int running_count = g_kernel.current_task_promise != nullptr ? 1 : 0;
    bool condition = task_count == running_count + ready_count + waiting_count + io_waiting_count + zombie_count; // -1 for the running task
    if (!condition) {
        std::printf("Task       count: %d\n", task_count);
        std::printf("----------------:--------\n"); 
        std::printf("Running    count: %d\n", running_count); 
        std::printf("Ready      count: %d\n", ready_count);
        std::printf("Waiting    count: %d\n", waiting_count);
        std::printf("IO waiting count: %d\n", io_waiting_count);
        std::printf("Zombie     count: %d\n", zombie_count);
        assert(false);  
    }
}