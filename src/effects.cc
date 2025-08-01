
#include "task.hpp"

TaskHdl SuspendOp::await_suspend(TaskHdl currentTask) const noexcept {
    assert(gKernel.currentTask.isValid());

    TaskPromise& currentPromise = currentTask.promise();

    if constexpr (DEFINED_DEBUG) {
        assert(gKernel.currentTask == currentTask);  
        assert(currentPromise.state == TaskState::RUNNING);
        assert(currentPromise.waitNode.detached());
        checkInvariants();
    }
    
    std::print("SuspendEffect: suspending task({})\n", (void*)&gKernel.currentTask.promise());
    std::print("Task({}): scheduleNextTask() suspending the current task\n", (void*)&currentPromise); 

    // Move the current task from RUNNINIG to READY
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    gKernel.readyList.pushBack(&currentPromise.waitNode);
    gKernel.currentTask.clear();
    checkInvariants();
    
    // Resume the SchedulerTask
    TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
    schedulerPromise.state = TaskState::RUNNING;
    schedulerPromise.waitNode.detach(); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    checkInvariants();

    assert(gKernel.currentTask.isValid());
    return TaskHdl::from_promise(schedulerPromise);
}

TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    assert(gKernel.currentTask == currentTaskHdl);
    
    std::print("Task({0})::resume(): requested to resume Task({1})\n", (void*)&gKernel.currentTask.promise(), (void*)&hdl.promise());
    
    // Check the current Task
    TaskPromise& currentPromise = gKernel.currentTask.promise(); 
    assert(currentPromise.waitNode.detached());
    assert(currentPromise.state == TaskState::RUNNING); 
    checkInvariants();

    // Suspend the current Task
    currentPromise.state = TaskState::READY;
    ++gKernel.readyCount;
    gKernel.readyList.pushBack(&currentPromise.waitNode);
    gKernel.currentTask = TaskHdl();
    checkInvariants();

    // Move the target task from READY to RUNNING
    TaskPromise& promise = hdl.promise();
    promise.state = TaskState::RUNNING;
    promise.waitNode.detach();
    --gKernel.readyCount;
    gKernel.currentTask = hdl;
    checkInvariants();

    assert(gKernel.currentTask.isValid());
    return hdl;
}