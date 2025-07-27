
#include "task.hpp"



TaskHdl SuspendEffect::await_suspend(TaskHdl current_task_hdl) const noexcept {
    //
    // TODO: Comprire bene tutti i casi
    //

    TaskPromise& current_promise = current_task_hdl.promise();

    // Ensure that the current task is RUNNING and that it is correctly set as the current task
    assert(g_kernel.current_task_promise == &current_promise);
    assert(current_promise.state == TaskState::RUNNING);
    assert(current_promise.wait_node.detached());
    checkTaskCountInvariant();
    
    std::printf("SuspendEffect: suspending task(%p)\n", g_kernel.current_task_promise);
    std::printf("Task(%p): scheduleNextTask() suspending the current task\n", &current_promise); 

    // Move the current task from RUNNINIG to READY
    current_promise.state = TaskState::READY;
    ++g_kernel.ready_count;
    g_kernel.ready_list.push_back(&current_promise.wait_node);
    g_kernel.current_task_promise = nullptr;
    checkTaskCountInvariant();
    
    // Resume the SchedulerTask
    TaskPromise& scheduler_promise = *g_kernel.scheduler_task_promise;
    scheduler_promise.state = TaskState::RUNNING;
    scheduler_promise.wait_node.detach(); // remove from ready list
    --g_kernel.ready_count;
    g_kernel.current_task_promise = &scheduler_promise;
    checkTaskCountInvariant();

    return TaskHdl::from_promise(scheduler_promise);
}

