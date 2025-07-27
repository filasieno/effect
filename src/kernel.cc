#include "task.hpp"

struct Kernel g_kernel;

static void initKernel() noexcept;
static void finiKernel() noexcept; 


struct KernelBootPromise;

using KernelBootTaskHdl = std::coroutine_handle<KernelBootPromise>;

struct ExecuteSchedulerTaskEffect {

    ExecuteSchedulerTaskEffect(TaskHdl schedulerHdl) : schedulerHdl(schedulerHdl) {};

    bool await_ready() const noexcept {
        return schedulerHdl.done();
    }

    TaskHdl await_suspend(KernelBootTaskHdl currentTaskHdl) const noexcept {
        (void)currentTaskHdl;
        
        TaskPromise& scheduler_promise = schedulerHdl.promise();

        // Check expected state post scheduler construction
        assert(g_kernel.task_count == 1);
        assert(g_kernel.ready_count == 1);
        assert(scheduler_promise.state == TaskState::READY);
        assert(g_kernel.current_task_promise == nullptr);

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        g_kernel.current_task_promise = &scheduler_promise;
        scheduler_promise.state = TaskState::RUNNING;
        scheduler_promise.wait_node.detach();
        --g_kernel.ready_count;
    
        // Check expected state post task system bootstrap
        checkTaskCountInvariant();

        return schedulerHdl;
    }

    void await_resume() const noexcept {}

    TaskHdl schedulerHdl;
};

struct SchedulerTask {
    using promise_type = TaskPromise;
    
    SchedulerTask(TaskHdl hdl) noexcept : hdl(hdl) {}

    ~SchedulerTask() noexcept {
        hdl.destroy();
    }

    bool done() const noexcept {
        return hdl.done();
    }

    TaskState state() const noexcept;

    TaskHdl hdl;
};

static SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept;

struct KernelBootPromise {

    KernelBootPromise(std::function<Task()> user_main_task) : user_main_task(user_main_task){}
    ~KernelBootPromise() {}
    
    KernelBootTaskHdl get_return_object() noexcept {
        return KernelBootTaskHdl::from_promise(*this);
    }

    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void               return_void() noexcept {}
    void               unhandled_exception() noexcept { assert(false); }

    std::function<Task()> user_main_task;
};

struct KernelBootTask {
    using promise_type = KernelBootPromise;
    
    KernelBootTask(KernelBootTaskHdl hdl) noexcept : hdl(hdl) {}

    ~KernelBootTask() noexcept {
        hdl.destroy();
    }

    void run() {
        hdl.resume();
    }

    KernelBootTaskHdl hdl;
};

KernelBootTask startKernelBootTask(std::function<Task()> user_main_task) noexcept {
    SchedulerTask scheduler_task = scheduler(user_main_task);
    std::printf("startMainTask(std::function<Task()>): Scheduler is Task(%p)\n", &scheduler_task.hdl.promise());
    g_kernel.scheduler_task_promise = &scheduler_task.hdl.promise();
    co_await ExecuteSchedulerTaskEffect(scheduler_task.hdl);
    
    // The scheduler task never returns; it runs until completion
    assert(scheduler_task.hdl.done());

    co_return;
}

int startMainTask(std::function<Task()> user_main_task) noexcept {
    initKernel();

    KernelBootTask kernel_boot_task = startKernelBootTask(user_main_task);
    kernel_boot_task.run();
        
    finiKernel();
    return 0; 
}

void initKernel() noexcept {
    g_kernel.task_count = 0;
    g_kernel.ready_count = 0;
    g_kernel.waiting_count = 0;
    g_kernel.io_waiting_count = 0;
    g_kernel.zombie_count = 0;
    g_kernel.interrupted = 0;
    g_kernel.current_task_promise = nullptr;
    g_kernel.zombie_list.init();
    g_kernel.ready_list.init();
    g_kernel.task_list.init();
    std::printf("Kernel::init(): initialized\n");

    // Check invariants
    checkTaskCountInvariant();
}

void finiKernel() noexcept {
    // TODO: add checks to ensure all tasks are finalized

    std::printf("Kernel::fini(): finalized\n");
    
    // Check invariants
    checkTaskCountInvariant();
}

static int started = 0;

SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept {    
    ++started;
    std::fflush(stdout);
    assert(started == 1);

    std::printf(">> SchedulerTask(%p): started\n", g_kernel.current_task_promise);
    std::printf(">> SchedulerTask(%p): about to spawn main task\n", g_kernel.current_task_promise); // g_kernel.current_task_promise 
    Task main_task = user_main_task();
    std::printf(">> SchedulerTask(%p): Main task is Task(%p)\n", g_kernel.current_task_promise, &main_task.hdl.promise());
    assert(!main_task.hdl.done());
    assert(main_task.state() == TaskState::READY);
    std::printf(">> SchedulerTask(%p): did spawn main task\n", g_kernel.current_task_promise);

    while (true) {
        // If we have a ready task, resume it
        if (g_kernel.ready_count > 0) {
            DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
            TaskPromise* next_ready_promise = waitListNodeToTask(next_ready_promise_node);

            // check invariants
            TaskPromise& scheduler_promise = *g_kernel.scheduler_task_promise;
            assert(next_ready_promise->state == TaskState::READY);
            assert(g_kernel.current_task_promise == &scheduler_promise);
            assert(scheduler_promise.wait_node.detached());
            checkTaskCountInvariant();

            // move the scheduler task from RUNNING to READY
            scheduler_promise.state = TaskState::READY;
            ++g_kernel.ready_count;
            g_kernel.ready_list.push_back(&scheduler_promise.wait_node);
            g_kernel.current_task_promise = nullptr;
            checkTaskCountInvariant();
            
            co_await ExecuteTaskEffect(TaskHdl::from_promise(*next_ready_promise));
            continue;
        }
    }
    co_return;
}


TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}