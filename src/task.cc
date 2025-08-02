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
    using namespace internal_ak;

    InitLink(&taskList);
    InitLink(&waitNode);    
    InitLink(&waitTaskNode);
    state = TaskState::CREATED;
    std::print("TaskPromise::TaskPromise(): Task({}) initialized\n", (void*)this); 
    
    // Check post-conditions
    assert(IsLinkDetached(&taskList));
    assert(IsLinkDetached(&waitNode));
    assert(state == TaskState::CREATED);
    CheckInvariants();
}

TaskPromise::~TaskPromise() {
    using namespace internal_ak;
    std::print("TaskPromise::TaskPromise(): about to finalize Task({})\n", (void*)this); 
    assert(state == TaskState::DELETING);
    assert(IsLinkDetached(&taskList));
    assert(IsLinkDetached(&waitNode));

    std::print("TaskPromise::TaskPromise(): did finalize Task({})\n", (void*)this); 
    DebugTaskCount();
    CheckInvariants();
}

TaskHdl TaskPromise::get_return_object() noexcept { 
    std::print("TaskPromise::TaskPromise(): Task({}) returning TaskHdl\n", (void*)this);
    return TaskHdl::from_promise(*this);
}

void TaskPromise::return_void() noexcept {
    using namespace internal_ak;
    std::print("TaskPromise::TaskPromise(): Task({}) returned void\n", (void*)this);
    CheckInvariants();
    
    // Wake up all tasks waiting for this task
    if (IsLinkDetached(&waitTaskNode)) {
        std::print("TaskPromise::TaskPromise(): Task({}) there are no waiting tasks\n", (void*)this);
        return;
    }

    std::print("TaskPromise::TaskPromise(): Task({}) waking up waiting tasks\n", (void*)this);
    do {
        Link* next = DequeueLink(&waitTaskNode);
        TaskPromise& nextPromise = *waitListNodeToTaskPromise(next);

        std::print("TaskPromise::TaskPromise(): Task({}) about to wake up Task({})\n", (void*)this, (void*)&nextPromise); 
        DebugTaskCount();

        assert(nextPromise.state == TaskState::WAITING);
        --gKernel.waitingCount;
        nextPromise.state = TaskState::READY;
        EnqueueLink(&gKernel.readyList, &nextPromise.waitNode);
        ++gKernel.readyCount;
        
        std::print("TaskPromise::TaskPromise(): Task({}) did wake up Task({})\n", (void*)this, (void*)&nextPromise);
        DebugTaskCount();

    } while (!IsLinkDetached(&waitTaskNode));
}

// TaskPromise::InitialSuspend -------------------------------------------------

void TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl) const noexcept {
    using namespace internal_ak;
    TaskPromise& promise = hdl.promise();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): initial suspend started({})\n", (void*)&promise);  
    
    // Check initial preconditions
    assert(promise.state == TaskState::CREATED);
    assert(IsLinkDetached(&promise.waitNode));
    CheckInvariants();

    // Add task to the kernel
    ++gKernel.taskCount;    
    EnqueueLink(&gKernel.taskList, &promise.taskList);

    ++gKernel.readyCount;
    EnqueueLink(&gKernel.readyList, &promise.waitNode);
    promise.state = TaskState::READY;

    // Check post-conditions
    assert(promise.state == TaskState::READY);
    assert(!IsLinkDetached(&promise.waitNode));
    CheckInvariants();

    std::print("TaskPromise::InitialSuspend::await_suspend(TaskHdl hdl): newly spawned Task({}) is {}\n", (void*)&promise, (int)promise.state); 
    internal_ak::DebugTaskCount();
}


// TaskPromise::FinalSuspend -------------------------------------------------

TaskHdl TaskPromise::FinalSuspend::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    // Check preconditions
    TaskPromise& currentPromise = currentTaskHdl.promise();                            
    assert(gKernel.currentTask == currentTaskHdl);
    assert(currentPromise.state == TaskState::RUNNING);
    assert(IsLinkDetached(&currentPromise.waitNode));
    internal_ak::CheckInvariants();
    
    // Move the current task from RUNNING to ZOMBIE
    currentPromise.state = TaskState::ZOMBIE;
    ++gKernel.zombieCount;
    EnqueueLink(&gKernel.zombieList, &currentPromise.waitNode);
    gKernel.currentTask.clear();
    internal_ak::CheckInvariants();

    // Move the SchedulerTask from READY to RUNNING
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    schedulerPromise.state = TaskState::RUNNING;    
    DetachLink(&schedulerPromise.waitNode); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    internal_ak::CheckInvariants();
    
    return gKernel.schedulerTask;
}

// -----------------------------------------------------------------------------
// Debug utilities
// -----------------------------------------------------------------------------
namespace internal_ak {

    void DebugTaskCount() noexcept {
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

    inline static void DoCheckTaskCountInvariant() noexcept {
        int running_count = gKernel.currentTask != TaskHdl() ? 1 : 0;
        bool condition = gKernel.taskCount == running_count + gKernel.readyCount + gKernel.waitingCount + gKernel.ioWaitingCount + gKernel.zombieCount; // -1 for the running task
        if (!condition) {
            DebugTaskCount();
            std::abort();
        }
    }

    void CheckTaskCountInvariant() noexcept {
        if constexpr (DEFINED_DEBUG) {
            DoCheckTaskCountInvariant();
        }
    }

    void CheckInvariants() noexcept {
        if constexpr (DEFINED_DEBUG) {
            // check the Task invariants
            DoCheckTaskCountInvariant();

            // Ensure that the current Task is valid
            // if (gKernel.currentTask.isValid()) std::abort();
        }    
    }

}

