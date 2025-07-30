#include "task.hpp"

// -----------------------------------------------------------------------------
// TaskPromise
// -----------------------------------------------------------------------------

void* TaskPromise::operator new(std::size_t n) noexcept {
    std::print("TaskPromise: About to allocate a Task of size: {}\n", n);
    void* mem = std::malloc(n);
    if (!mem)
        return nullptr;
    unsigned long long frame_head = (unsigned long long)mem;
    unsigned long long frame_tail = frame_head + n;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: Did allocate a Task({}) of size: {}\n", (void*)task_addr, n); 
    return mem;
}

void TaskPromise::operator delete(void* ptr, std::size_t sz) {
    unsigned long long frame_head = (unsigned long long)ptr;
    unsigned long long frame_tail = frame_head + sz;
    unsigned long long task_addr  = frame_tail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: About to free Task({}) of size: {}\n", (void*)task_addr, sz);  
    
    std::free(ptr);
    std::print("TaskPromise: Did free Task({}) of size: {}\n", (void*)task_addr, sz);   
}

TaskPromise::TaskPromise() {
    taskList.init();
    waitNode.init();
    waitingTaskNode.init();
    state = TaskState::CREATED;
    std::print("TaskPromise::TaskPromise(): Task({}) initialized\n", (void*)this); 
    
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
    std::print("TaskPromise::TaskPromise(): Task({}) finalized\n", (void*)this);

    // todo: move await from destructor
    --gKernel.taskCount;
    --gKernel.zombieCount;
    checkTaskCountInvariant();
}

TaskHdl TaskPromise::get_return_object() noexcept { 
    std::print("TaskPromise::TaskPromise(): Task({}) returning TaskHdl\n", (void*)this);
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

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started({})\n", (void*)&promise);  
    
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

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task({}) is {}\n", (void*)&promise, (int)promise.state); 
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
    std::print("----------------:--------\n"); 
    std::print("Task       count: {}\n", gKernel.taskCount);
    std::print("----------------:--------\n"); 
    std::print("Running    count: {}\n", running_count); 
    std::print("Ready      count: {}\n", gKernel.readyCount);
    std::print("Waiting    count: {}\n", gKernel.waitingCount);
    std::print("IO waiting count: {}\n", gKernel.ioWaitingCount);
    std::print("Zombie     count: {}\n", gKernel.zombieCount);
    std::print("----------------:--------\n"); 
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



