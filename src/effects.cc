
#include "task.hpp"

TaskHdl SuspendOp::await_suspend(TaskHdl current_task_hdl) const noexcept {
    //
    // TODO: Comprire bene tutti i casi
    //

    TaskPromise& current_promise = current_task_hdl.promise();

    if constexpr (DEFINED_DEBUG) {
        assert(gKernel.currentTask == current_task_hdl);  
        assert(current_promise.state == TaskState::RUNNING);
        assert(current_promise.waitNode.detached());
        checkTaskCountInvariant();
    }
    
    std::print("SuspendEffect: suspending task({})\n", (void*)&gKernel.currentTask.promise());
    std::print("Task({}): scheduleNextTask() suspending the current task\n", (void*)&current_promise); 

    // Move the current task from RUNNINIG to READY
    current_promise.state = TaskState::READY;
    ++gKernel.readyCount;
    gKernel.readyList.pushBack(&current_promise.waitNode);
    gKernel.currentTask = TaskHdl();
    checkTaskCountInvariant();
    
    // Resume the SchedulerTask
    TaskPromise& scheduler_promise = gKernel.schedulerTask.promise();
    scheduler_promise.state = TaskState::RUNNING;
    scheduler_promise.waitNode.detach(); // remove from ready list
    --gKernel.readyCount;
    gKernel.currentTask = gKernel.schedulerTask;
    checkTaskCountInvariant();

    return TaskHdl::from_promise(scheduler_promise);
}

TaskHdl ResumeTaskOp::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    assert(gKernel.currentTask == currentTaskHdl);
    
    std::print("Task({0})::resume(): requested to resume Task({1})\n", (void*)&gKernel.currentTask.promise(), (void*)&hdl.promise());
    
    // Check the current Task
    TaskPromise& current_task_promise = gKernel.currentTask.promise(); 
    assert(current_task_promise.waitNode.detached());
    assert(current_task_promise.state == TaskState::RUNNING); 
    checkTaskCountInvariant();

    // Suspend the current Task
    current_task_promise.state = TaskState::READY;
    ++gKernel.readyCount;
    gKernel.readyList.pushBack(&current_task_promise.waitNode);
    gKernel.currentTask = TaskHdl();
    checkTaskCountInvariant();

    // Move the target task from READY to RUNNING
    TaskPromise& target_task_promise = hdl.promise();
    target_task_promise.state = TaskState::RUNNING;
    target_task_promise.waitNode.detach();
    --gKernel.readyCount;
    gKernel.currentTask = hdl;
    checkTaskCountInvariant();

    return hdl;
}