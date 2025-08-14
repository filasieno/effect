# Core Module Documentation

## TaskContext

**Brief:** Define a context.

**Group:** Task

The TaskContext struct represents the context for a task coroutine. It manages the task's state, I/O operations, and various linked lists for task management.

## DefineTask

**Brief:** Marks a Task coroutine function

The DefineTask struct is used to mark and define task coroutine functions. It wraps a TaskHdl and provides the necessary promise_type for coroutine functionality.

## KernelConfig

**Brief:** Configuration for the Kernel

**Group:** Kernel

The KernelConfig struct contains configuration parameters for initializing the kernel, including memory allocation and I/O entry count settings.

## Functions

### ClearTaskHdl

**Brief:** Clears the target TaskHdl

**Parameters:**

- `hdl` - the handle to be cleared

**Group:** Task

Resets a TaskHdl to its default (empty) state.

### IsTaskHdlValid

**Brief:** Checks is the a TaskHdl is valid

**Parameters:**

- `hdl` - the handle to be cleared

**Group:** Task

Returns true if the TaskHdl points to a valid task (non-null address).

### GetTaskContext (with parameter)

**Brief:** Returns the TaskPromise associated with the target TaskHdl

**Parameters:**
- `hdl` - the task handle

**Returns:** the TaskPromise associated with the target TaskHdl

**Group:** Task

Retrieves the TaskContext (promise) from a given TaskHdl.

### GetTaskContext (no parameters)

**Brief:** Returns the TaskPromise associated with the target TaskHdl

**Parameters:**

- `hdl` - the task handle

**Returns:** the TaskPromise associated with the target TaskHdl

**Group:** Task

Retrieves the TaskContext (promise) for the currently executing task.

### GetCurrentTask

**Brief:** Get the current Task

**Returns:** [Async] TaskHdl

**Group:** Task

Asynchronously retrieves a handle to the currently executing task.

### SuspendTask

**Brief:** Suspends the current Task and resumes the Scheduler.

**Returns:** [Async] void

**Group:** Task

Suspends the current task execution and yields control back to the scheduler.

### JoinTask

**Brief:** Suspends the current Task until the target Task completes.

**Parameters:**

- `hdl` - a handle to the target Task.

**Returns:** [Async] void

**Group:** Task

Waits for the specified task to complete before resuming execution.

### operator co_await

**Brief:** Alias for AkJoinTask

**Parameters:**

- `hdl` - a handle to the target Task.

**Returns:** [Async] void

**Group:** Task

Provides co_await syntax support for joining tasks.

### GetTaskState

**Brief:** Resturns the current TaskState.

**Parameters:**
- `hdl` - a handle to the target Task.

**Returns:** the current TaskState

**Group:** Task

Retrieves the current state of the specified task.

### IsTaskDone

**Brief:** Returns true if the target Task is done.

**Parameters:**

- `hdl` - a handle to the target Task

**Returns:** `true` if the target Task is done

**Group:** Task

Checks whether the specified task has completed execution.

### ResumeTask

**Brief:** Resumes a Task that is in TaskState::READY

**Parameters:**

- `hdl` - a handle to the target Task

**Returns:** true if the target Task is done

**Group:** Task

Resumes execution of a task that is in the READY state.
