# Why Your Coroutine is Restarting Instead of Continuing

## The Root Cause

Looking at your code and error log, the problem is that your `SuspendEffect::await_suspend()` is performing a **symmetric transfer** correctly, but the scheduler coroutine is being **restarted from the beginning** instead of **resuming from where it left off**.

In your `scheduler()` function in `kernel.cc`, after you resume the main task, you have:

```cpp
// Resume the target task
main_task.hdl.resume();

std::printf("========= Entering =======\n");
std::fflush(stdout);            
assert(false);  // <-- This line is never reached in normal flow
```

The issue is that when `main_task.hdl.resume()` is called, it eventually leads to `SuspendEffect::await_suspend()` which performs a symmetric transfer back to the scheduler. However, the scheduler coroutine **starts executing from the beginning again** instead of continuing after the `main_task.hdl.resume()` call.

## Why This Happens

The problem is in your scheduler function structure. Your scheduler is a coroutine, but you're calling `main_task.hdl.resume()` directly instead of using `co_await`. When the symmetric transfer returns control to the scheduler, the coroutine machinery doesn't know where to resume from because the suspension point isn't properly established.

**Key insight**: Symmetric transfers work between coroutine suspension points (`co_await`), not arbitrary function calls. You need to establish proper `co_await` points in your scheduler for the symmetric transfer to work correctly.

## Why Your Current Approach Fails

The error shows `started: 2` because when the symmetric transfer returns control to the scheduler coroutine, it's not resuming from the suspension point but rather **restarting the entire coroutine function from the beginning**. This happens because:

1. The scheduler coroutine doesn't have a proper suspension point when calling `main_task.hdl.resume()`
2. The symmetric transfer returns to the scheduler, but there's no established coroutine frame state to resume from
3. The coroutine machinery treats this as a fresh start

## The Fix

You need to restructure your scheduler to use proper coroutine suspension points. Here's what you should do:

### 1. Make the scheduler properly await tasks

Replace your current scheduler implementation with:

```cpp
SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept {    
    ++started;
    std::printf(">> started: %d\n", started);
    std::fflush(stdout);
    assert(started == 1);
    
    std::printf(">> SchedulerTask(%p): started\n", g_kernel.current_task_promise);
    std::printf(">> SchedulerTask(%p): about to spawn main task\n", g_kernel.current_task_promise);
    
    Task main_task = user_main_task();
    std::printf(">> SchedulerTask(%p): Main task is Task(%p)\n", g_kernel.current_task_promise, &main_task.hdl.promise());
    
    // Instead of manually managing task switching, use co_await
    co_await main_task;  // This will properly suspend the scheduler
    
    // Now handle the scheduler loop
    while (true) {
        if (g_kernel.ready_count > 0) {
            DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
            TaskPromise* next_ready_promise = wait_node_to_promise(next_ready_promise_node);
            
            // Create a Task wrapper and await it
            Task next_task = Task(TaskHdl::from_promise(*next_ready_promise));
            co_await next_task;
        } else {
            break; // No more tasks
        }
    }
    co_return;
}
```

### 2. Make Task awaitable

You'll need to implement an awaiter for `Task` so it can be used with `co_await`. Add this to your `Task` struct in `task.hpp`:

```cpp
struct Task {
    using promise_type = TaskPromise;

    Task() = default;
    Task(TaskHdl hdl) : hdl(hdl) {}

    // Make Task awaitable
    bool await_ready() const noexcept {
        return hdl.done();
    }
    
    TaskHdl await_suspend(TaskHdl current) const noexcept {
        // Check invariants
        checkTaskCountInvariant();
        
        if (hdl.done()) {
            // Task is already done, don't suspend
            return current;
        }
        
        std::printf("Task(%p)::await_suspend(): requested to resume Task(%p)\n", 
                   &current.promise(), &hdl.promise());
        
        // Ensure that the target task is READY
        TaskPromise* target_promise = &hdl.promise();
        assert(target_promise->state == TaskState::READY);
        assert(!target_promise->wait_node.detached());

        // Ensure that the current task is RUNNING
        TaskPromise* current_promise = &current.promise();
        assert(current_promise != nullptr);
        assert(current_promise->state == TaskState::RUNNING);
        assert(current_promise->wait_node.detached());

        // Move the current task from RUNNING to READY
        current_promise->state = TaskState::READY;
        ++g_kernel.ready_count;
        g_kernel.ready_list.push_back(&current_promise->wait_node);
        
        // Move the target task from READY to RUNNING
        --g_kernel.ready_count;
        target_promise->wait_node.detach();
        target_promise->state = TaskState::RUNNING;
        g_kernel.current_task_promise = target_promise;

        std::printf("Task(%p)::await_suspend(): is the new RUNNING Task\n", 
                   g_kernel.current_task_promise);
        
        // Check post-conditions
        assert(g_kernel.current_task_promise == target_promise);
        assert(g_kernel.current_task_promise->state == TaskState::RUNNING);
        assert(g_kernel.current_task_promise->wait_node.detached());
        checkTaskCountInvariant();

        // Return the target task handle for symmetric transfer
        return hdl;
    }
    


```
struct SchedulerTask {
    // ... existing members ...

    auto operator co_await() const noexcept {
        struct Awaitable {
            std::coroutine_handle<TaskPromise> hdl;
            
            bool await_ready() const noexcept { return hdl.done(); }
            void await_suspend(std::coroutine_handle<> h) const noexcept {
                // Setup suspension logic
            }
            void await_resume() const noexcept { }
        };
        return Awaitable{hdl};
    }
};    
```

