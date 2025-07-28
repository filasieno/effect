
#include "task.hpp"

TaskHdl SuspendEffect::await_suspend(TaskHdl current_task_hdl) const noexcept {
    //
    // TODO: Comprire bene tutti i casi
    //

    TaskPromise& current_promise = current_task_hdl.promise();

    if constexpr (DEFINED_DEBUG) {
        assert(g_kernel.current_task_hdl == current_task_hdl);  
        assert(current_promise.state == TaskState::RUNNING);
        assert(current_promise.wait_node.detached());
        checkTaskCountInvariant();
    }
    
    std::printf("SuspendEffect: suspending task(%p)\n", &g_kernel.current_task_hdl.promise());
    std::printf("Task(%p): scheduleNextTask() suspending the current task\n", &current_promise); 

    // Move the current task from RUNNINIG to READY
    current_promise.state = TaskState::READY;
    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&current_promise.wait_node);
    g_kernel.current_task_hdl = TaskHdl();
    checkTaskCountInvariant();
    
    // Resume the SchedulerTask
    TaskPromise& scheduler_promise = g_kernel.scheduler_task_hdl.promise();
    scheduler_promise.state = TaskState::RUNNING;
    scheduler_promise.wait_node.detach(); // remove from ready list
    --g_kernel.ready_count;
    g_kernel.current_task_hdl = g_kernel.scheduler_task_hdl;
    checkTaskCountInvariant();

    return TaskHdl::from_promise(scheduler_promise);
}

TaskHdl ExecuteTaskEffect::await_suspend(TaskHdl currentTaskHdl) const noexcept {
    assert(g_kernel.current_task_hdl == currentTaskHdl);
    
    std::print("Task({0})::resume(): requested to resume Task({1})\n", (void*)&g_kernel.current_task_hdl.promise(), (void*)&hdl.promise());
    
    // Check the current Task
    TaskPromise& current_task_promise = g_kernel.current_task_hdl.promise(); 
    assert(current_task_promise.wait_node.detached());
    assert(current_task_promise.state == TaskState::RUNNING); 
    checkTaskCountInvariant();

    // Suspend the current Task
    current_task_promise.state = TaskState::READY;
    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&current_task_promise.wait_node);
    g_kernel.current_task_hdl = TaskHdl();
    checkTaskCountInvariant();

    // Move the target task from READY to RUNNING
    TaskPromise& target_task_promise = hdl.promise();
    target_task_promise.state = TaskState::RUNNING;
    target_task_promise.wait_node.detach();
    --g_kernel.ready_count;
    g_kernel.current_task_hdl = hdl;
    checkTaskCountInvariant();

    return hdl;
}