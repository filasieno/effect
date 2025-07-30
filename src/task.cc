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
    taskList.init();
    waitNode.init();
    waitingTaskNode.init();
    state = TaskState::CREATED;
    std::printf("TaskPromise::TaskPromise(): Task(%p) initialized\n", this); 
    
    // Check post-conditions
    assert(taskList.detached());
    assert(waitNode.detached());
    assert(state == TaskState::CREATED);
    checkTaskCountInvariant();
}

TaskPromise::~TaskPromise() {
    assert(state == TaskState::ZOMBIE);
    taskList.detach();
    waitNode.detach();
    waitingTaskNode.detach();
    std::printf("TaskPromise::TaskPromise(): Task(%p) finalized\n", this);

    // todo: move await from destructor
    --gKernel.taskCount;
    --gKernel.zombieCount;
    checkTaskCountInvariant();
}

TaskHdl TaskPromise::get_return_object() noexcept { 
    std::printf("TaskPromise::TaskPromise(): Task(%p) returning TaskHdl\n", this);
    return TaskHdl::from_promise(*this);
}

void TaskPromise::return_void() noexcept {
    std::print("TaskPromise::TaskPromise(): Task({}) returned void\n", (void*)this);
    checkTaskCountInvariant();
    
    // Wake up all tasks waiting for this task
    if (waitingTaskNode.detached()) {
        std::print("TaskPromise::TaskPromise(): Task({}) there are no waiting tasks\n", (void*)this);
        return;
    }

    std::print("TaskPromise::TaskPromise(): Task({}) waking up waiting tasks\n", (void*)this);
    do {
        DList* next_waiting_task = waitingTaskNode.pop_front();
        TaskPromise* next_waiting_task_promise = waitListNodeToTaskPromise(next_waiting_task);
        std::print("TaskPromise::TaskPromise(): Task({}) did wake up Task({})\n", (void*)this, (void*)next_waiting_task_promise);
        assert(next_waiting_task_promise->state == TaskState::WAITING);
        --gKernel.waitingCount;
        next_waiting_task_promise->state = TaskState::READY;
        gKernel.readyList.push_back(&next_waiting_task_promise->waitNode);
        ++gKernel.readyCount;
    } while (!waitingTaskNode.detached());
}

// TaskPromise::InitialSuspend -------------------------------------------------

void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& promise = hdl.promise();

    std::printf("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started(%p)\n", (void*)&promise);  
    
    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(promise.waitNode.detached());
    checkTaskCountInvariant();

    // Add task to the kernel
    ++gKernel.taskCount;    
    gKernel.taskList.push_back(&promise.taskList);

    ++gKernel.readyCount;
    gKernel.readyList.push_back(&promise.waitNode);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!promise.waitNode.detached()); 
    checkTaskCountInvariant();

    std::printf("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task(%p) is %d\n", (void*)&promise, promise.state); 
    debugTaskCount();
}


// TaskPromise::FinalSuspend -------------------------------------------------

TaskHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& currentTaskPromise = hdl.promise();                            
    
    // Check preconditions
    assert(gKernel.currentTask == hdl);
    assert(currentTaskPromise.state == TaskState::RUNNING);
    assert(currentTaskPromise.waitNode.detached());
    checkTaskCountInvariant();
    
     // Check the current Task
    TaskPromise& current_task_promise = hdl.promise();
    assert(current_task_promise.waitNode.detached());
    assert(current_task_promise.state == TaskState::RUNNING); 
    checkTaskCountInvariant();

    // Move the current task from RUNNING to ZOMBIE
    current_task_promise.state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    gKernel.zombieList.push_back(&current_task_promise.waitNode);
    gKernel.currentTask.clear();
    checkTaskCountInvariant();

    // Move the SchedulerTask from READY to RUNNING
    TaskPromise& scheduler_promise = gKernel.schedulerTask.promise();
    scheduler_promise.state = TaskState::RUNNING;
    scheduler_promise.waitNode.detach(); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    checkTaskCountInvariant();
    
    return gKernel.schedulerTask;
}

// -----------------------------------------------------------------------------
// Debug utilities
// -----------------------------------------------------------------------------

void debugTaskCount() noexcept {
    int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
    std::printf("----------------:--------\n"); 
    std::printf("Task       count: %d\n", gKernel.taskCount);
    std::printf("----------------:--------\n"); 
    std::printf("Running    count: %d\n", running_count); 
    std::printf("Ready      count: %d\n", gKernel.readyCount);
    std::printf("Waiting    count: %d\n", gKernel.waitingCount);
    std::printf("IO waiting count: %d\n", gKernel.ioWaitingCount);
    std::printf("Zombie     count: %d\n", gKernel.zombieCount);
    std::printf("----------------:--------\n"); 
}

void checkTaskCountInvariant() noexcept {
    if (DEFINED_DEBUG) {
        int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
        bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount; // -1 for the running task
        if (!condition) {
            debugTaskCount();
            std::abort();
        }
    }    
}



