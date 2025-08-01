#include "task.hpp"

// -----------------------------------------------------------------------------
// TaskPromise
// -----------------------------------------------------------------------------

void* TaskPromise::operator new(std::size_t n) noexcept {
    std::print("TaskPromise: About to allocate a Task of size: {}\n", n);
    void* mem = std::malloc(n);
    if (!mem)
        return nullptr;
    unsigned long long frameHead = (unsigned long long)mem;
    unsigned long long frameTail = frameHead + n;
    unsigned long long taskAddr  = frameTail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: Did allocate a Task({}) of size: {}\n", (void*)taskAddr, n); 
    return mem;
}

void TaskPromise::operator delete(void* ptr, std::size_t sz) {
    unsigned long long frameHead = (unsigned long long)ptr;
    unsigned long long frameTail = frameHead + sz;
    unsigned long long taskAddr  = frameTail - sizeof(TaskPromise) - 8;
    std::print("TaskPromise: About to free Task({}) of size: {}\n", (void*)taskAddr, sz);  
    
    std::free(ptr);
    std::print("TaskPromise: Did free Task({}) of size: {}\n", (void*)taskAddr, sz);   
}

TaskPromise::TaskPromise() {
    taskList.init();
    waitNode.init();
    terminatedEvent.init();
    state = TaskState::CREATED;
    std::print("TaskPromise::TaskPromise(): Task({}) initialized\n", (void*)this); 
    
    // Check post-conditions
    assert(taskList.detached());
    assert(waitNode.detached());
    assert(state == TaskState::CREATED);
    checkInvariants();
}

TaskPromise::~TaskPromise() {
    std::print("TaskPromise::TaskPromise(): about to finalize Task({})\n", (void*)this); 
    assert(state == TaskState::DELETING);
    assert(taskList.detached());
    assert(waitNode.detached());

    std::print("TaskPromise::TaskPromise(): did finalize Task({})\n", (void*)this); 
    debugTaskCount();
    checkInvariants();
}

TaskHdl TaskPromise::get_return_object() noexcept { 
    std::print("TaskPromise::TaskPromise(): Task({}) returning TaskHdl\n", (void*)this);
    return TaskHdl::from_promise(*this);
}

void TaskPromise::return_void() noexcept {
    std::print("TaskPromise::TaskPromise(): Task({}) returned void\n", (void*)this);
    checkInvariants();
    
    // Wake up all tasks waiting for this task
    if (terminatedEvent.detached()) {
        std::print("TaskPromise::TaskPromise(): Task({}) there are no waiting tasks\n", (void*)this);
        return;
    }

    std::print("TaskPromise::TaskPromise(): Task({}) waking up waiting tasks\n", (void*)this);
    do {
        DList* next = terminatedEvent.popFront();
        TaskPromise& nextPromise = *waitListNodeToTaskPromise(next);

        std::print("TaskPromise::TaskPromise(): Task({}) about to wake up Task({})\n", (void*)this, (void*)&nextPromise); 
        debugTaskCount();

        assert(nextPromise.state == TaskState::WAITING);
        --gKernel.waitingCount;
        nextPromise.state = TaskState::READY;
        gKernel.readyList.pushBack(&nextPromise.waitNode);
        ++gKernel.readyCount;
        
        std::print("TaskPromise::TaskPromise(): Task({}) did wake up Task({})\n", (void*)this, (void*)&nextPromise);
        debugTaskCount();

    } while (!terminatedEvent.detached());
}

// TaskPromise::InitialSuspend -------------------------------------------------

void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    TaskPromise& promise = hdl.promise();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started({})\n", (void*)&promise);  
    
    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(promise.waitNode.detached());
    checkInvariants();

    // Add task to the kernel
    ++gKernel.taskCount;    
    gKernel.taskList.pushBack(&promise.taskList);

    ++gKernel.readyCount;
    gKernel.readyList.pushBack(&promise.waitNode);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!promise.waitNode.detached()); 
    checkInvariants();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task({}) is {}\n", (void*)&promise, (int)promise.state); 
    debugTaskCount();
}


// TaskPromise::FinalSuspend -------------------------------------------------

TaskHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    // Check preconditions
    TaskPromise& currentPromise = currentTaskHdl.promise();                            
    assert(gKernel.currentTask == currentTaskHdl);
    assert(currentPromise.state == TaskState::RUNNING);
    assert(currentPromise.waitNode.detached());
    checkInvariants();
    
    // Move the current task from RUNNING to ZOMBIE
    currentPromise.state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    gKernel.zombieList.pushBack(&currentPromise.waitNode);
    gKernel.currentTask.clear();
    checkInvariants();

    // Move the SchedulerTask from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    schedulerPromise.state = TaskState::RUNNING;
    schedulerPromise.waitNode.detach(); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    checkInvariants();
    
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

inline static void doCheckTaskCountInvariant() noexcept {
    int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
    bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount; // -1 for the running task
    if (!condition) {
        debugTaskCount();
        std::abort();
    }
}

void checkTaskCountInvariant() noexcept {
    if constexpr (DEFINED_DEBUG) {
        doCheckTaskCountInvariant();
    }
}

void checkInvariants() noexcept {
    if constexpr (DEFINED_DEBUG) {
        // check the Task invariants
        doCheckTaskCountInvariant();

        // Ensure that the current Task is valid
        // if (gKernel.currentTask.isValid()) std::abort();
    }    
}



