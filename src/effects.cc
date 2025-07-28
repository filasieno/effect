
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
    // Check invariants
    checkTaskCountInvariant();

    std::printf("Task(%p)::resume(): requested to resume Task(%p)\n", &g_kernel.current_task_hdl.promise(), &currentTaskHdl.promise());
    
    // Ensure that the target task is READY
    TaskPromise* target_promise = &currentTaskHdl.promise();
    assert(target_promise->state == TaskState::READY);
    assert(!target_promise->wait_node.detached());
    // todo: check that the target task is in the ready list

    // Ensure that the current task is RUNNING
    TaskPromise* current_promise = &g_kernel.current_task_hdl.promise();  
    if constexpr (DEFINED_DEBUG) {
        assert(current_promise != nullptr);
        assert(current_promise->state == TaskState::RUNNING);
        assert(current_promise->wait_node.detached());
    }

    // Move the current task from RUNNING to READY
    current_promise->state = TaskState::READY;
    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&current_promise->wait_node);
    
    // Move the target task from READY to RUNNING
    --g_kernel.ready_count;
    target_promise->wait_node.detach();
    target_promise->state = TaskState::RUNNING;
    g_kernel.current_task_hdl = currentTaskHdl;

    std::printf("Task(%p)::resume(): is the new RUNNING Task\n", &currentTaskHdl.promise());
    
    // Check post-conditions
    if constexpr (DEFINED_DEBUG) {
        assert(g_kernel.current_task_hdl == currentTaskHdl);
        assert(g_kernel.current_task_hdl.promise().state == TaskState::RUNNING);
        assert(g_kernel.current_task_hdl.promise().wait_node.detached());   
        checkTaskCountInvariant();
    }

    return hdl;
}