#include "task.hpp"

// -----------------------------------------------------------------------------
// TaskPromise
// -----------------------------------------------------------------------------

void* TaskPromise::operator new(std::size_t n) noexcept {
    printf("TaskPromise: About to allocate a Task of size: %zu\n", n);
    void* mem = std::malloc(n);
    if (!mem)
        return nullptr;
    unsigned long long frame_head = (unsigned long long)mem;
    unsigned long long frame_tail = frame_head + n;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    printf("TaskPromise: Did allocate a Task(%p) of size: %zu\n", (void*)task_addr, n); 
    return mem;
}

void TaskPromise::operator delete(void* ptr, std::size_t sz) {
    unsigned long long frame_head = (unsigned long long)ptr;
    unsigned long long frame_tail = frame_head + sz;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    printf("TaskPromise: About to free Task(%p) of size: %zu\n", (void*)task_addr, sz);  
    
    std::free(ptr);
    printf("TaskPromise: Did free Task(%p) of size: %zu\n", (void*)task_addr, sz);   
}

TaskPromise::TaskPromise() {
    task_list.init();
    wait_node.init();
    state = TaskState::CREATED;
    std::printf("TaskPromise::TaskPromise(): Task(%p) initialized\n", this); 
    
    // Check post-conditions
    assert(task_list.detached());
    assert(wait_node.detached());
    assert(state == TaskState::CREATED);
    checkTaskCountInvariant();
}

TaskPromise::~TaskPromise() {
    assert(state == TaskState::ZOMBIE);
    task_list.detach();
    wait_node.detach();
    std::printf("TaskPromise::TaskPromise(): Task(%p) finalized\n", this);

    // todo: move await from destructor
    --g_kernel.task_count;
    --g_kernel.zombie_count;
    checkTaskCountInvariant();
}

TaskHdl TaskPromise::get_return_object() noexcept { 
    std::printf("TaskPromise::TaskPromise(): Task(%p) returning TaskHdl\n", this);
    return TaskHdl::from_promise(*this);
}

void TaskPromise::return_void() noexcept {
    std::printf("TaskPromise::TaskPromise(): Task(%p) returned void\n", this);
    checkTaskCountInvariant();
}

// TaskPromise::InitialSuspend -------------------------------------------------

void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& promise = hdl.promise();

    std::printf("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started(%p)\n", (void*)&promise);  
    
    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(promise.wait_node.detached());
    checkTaskCountInvariant();

    // Add task to the kernel
    ++g_kernel.task_count;    
    g_kernel.task_list.push_back(&promise.task_list);

    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&promise.wait_node);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!promise.wait_node.detached()); 
    checkTaskCountInvariant();

    std::printf("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task(%p) is %d\n", (void*)&promise, promise.state); 
}


// TaskPromise::FinalSuspend -------------------------------------------------

CoroHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& current_promise = hdl.promise();                            
    
    // Check preconditions
    assert(current_promise.state == TaskState::RUNNING);
    assert(g_kernel.current_task_hdl == hdl);
    assert(current_promise.wait_node.detached());
    checkTaskCountInvariant();

    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): suspending Task(%p)\n", &current_promise);
    if (g_kernel.ready_count > 0) {
        std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): ready count > 0; ready count: %d\n", g_kernel.ready_count);
        // Remove current ready node
        current_promise.state = TaskState::ZOMBIE;
        ++g_kernel.zombie_count;
        g_kernel.zombie_list.push_back(&current_promise.wait_node);
        g_kernel.current_task_hdl = TaskHdl();
        
        // If there are ready tasks, we can just return the current handle
        DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
        TaskPromise* next_ready_promise = waitListNodeToTask(next_ready_promise_node);
        assert(next_ready_promise->state == TaskState::READY);
        next_ready_promise->wait_node.detach();
        --g_kernel.ready_count;
        g_kernel.current_task_hdl = TaskHdl::from_promise(*next_ready_promise);
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
    
    assert(g_kernel.scheduler_task_hdl == g_kernel.current_task_hdl); 
    std::printf("TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl): The scheduer is the last task running\n");
    g_kernel.interrupted = 1;
    g_kernel.scheduler_task_hdl.promise().state = TaskState::ZOMBIE;
    
    // Noop coroutine will force a symetric transfer exit; use std::coroutine_handle<> as a more flexible return type
    return std::noop_coroutine();
}

// -----------------------------------------------------------------------------
// Debug utilities
// -----------------------------------------------------------------------------

void debugTaskCount() noexcept {
    int running_count = g_kernel.current_task_hdl != TaskHdl() ? 1 : 0;
    std::printf("----------------:--------\n"); 
    std::printf("Task       count: %d\n", g_kernel.task_count);
    std::printf("----------------:--------\n"); 
    std::printf("Running    count: %d\n", running_count); 
    std::printf("Ready      count: %d\n", g_kernel.ready_count);
    std::printf("Waiting    count: %d\n", g_kernel.waiting_count);
    std::printf("IO waiting count: %d\n", g_kernel.io_waiting_count);
    std::printf("Zombie     count: %d\n", g_kernel.zombie_count);
    std::printf("----------------:--------\n"); 
}

void checkTaskCountInvariant() noexcept {
    if (DEFINED_DEBUG) {
        int running_count = g_kernel.current_task_hdl != TaskHdl() ? 1 : 0;
        bool condition = g_kernel.task_count == running_count + g_kernel.ready_count + g_kernel.waiting_count + g_kernel.io_waiting_count + g_kernel.zombie_count; // -1 for the running task
        if (!condition) {
            debugTaskCount();
            std::abort();
        }
    }    
}



