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
        std::print("ExecuteSchedulerTaskEffect::await_suspend: about to suspend KernelBootTaskHdl({0})\n", (void*)&currentTaskHdl.promise()); 
        
        // Check expected state post scheduler construction
        
        assert(g_kernel.task_count == 1);
        assert(g_kernel.ready_count == 1);
        assert(scheduler_promise.state == TaskState::READY);
        assert(!scheduler_promise.wait_node.detached());
        assert(g_kernel.current_task_hdl == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        g_kernel.current_task_hdl = schedulerHdl;
        scheduler_promise.state = TaskState::RUNNING;
        scheduler_promise.wait_node.detach();
        --g_kernel.ready_count;
    
        // Check expected state post task system bootstrap
        checkTaskCountInvariant();
        std::print("ExecuteSchedulerTaskEffect::await_suspend: about to resume Task({0})\n", (void*)&schedulerHdl.promise());
        return schedulerHdl;
    }

    void await_resume() const noexcept {
        std::print("ExecuteSchedulerTaskEffect: await_resume\n");
    }

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

    KernelBootPromise(std::function<Task()> user_main_task) : user_main_task(user_main_task){
        std::print("KernelBootPromise({0}): initialized\n", (void*)this);  
    }
    ~KernelBootPromise() {
        std::print("KernelBootPromise({0}): finalized\n", (void*)this);  
    }
    
    KernelBootTaskHdl get_return_object() noexcept {
        std::print("KernelBootPromise({0})::get_return_object: returning handle\n", (void*)this);     
        return KernelBootTaskHdl::from_promise(*this);
    }

    std::suspend_always initial_suspend() noexcept { 
        std::print("KernelBootPromise({0})::initial_suspend (std::suspend_always)\n", (void*)this);    
        return {}; 
    }

    std::suspend_never final_suspend() noexcept { 
        std::print("KernelBootPromise({0})::final_suspend (std::suspend_never)\n", (void*)this);    
        return {}; 
    }

    void return_void() noexcept {
        std::print("KernelBootPromise({0})::return_void\n", (void*)this);    
    }

    void unhandled_exception() noexcept { assert(false); }

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

KernelBootTask mainKernelTask(std::function<Task()> user_main_task) noexcept {
    
    std::print("mainKernelTask(std::function<Task): started the main kernel task\n"); 
    
    // Spawn the SchedulerTask
    std::print("mainKernelTask(std::function<Task): about to spawn the SchedulerTask\n"); 
    SchedulerTask scheduler_task = scheduler(user_main_task);
    g_kernel.scheduler_task_hdl = scheduler_task.hdl;
    std::print("mainKernelTask(std::function<Task): did spawn SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise()); 

    std::print("mainKernelTask(std::function<Task): about to execute SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise());  
    co_await ExecuteSchedulerTaskEffect(scheduler_task.hdl);
    std::print("mainKernelTask(std::function<Task): did to execute SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise());  
    
    // The scheduler task never returns; it runs until completion
    assert(scheduler_task.hdl.done());

    std::print("mainKernelTask(std::function<Task): terminated the main kernel task\n"); 
    co_return;
}

int runMainTask(std::function<Task()> user_main_task) noexcept {
    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr); 

    std::print("runMainTask(): started ...\n"); 
    initKernel();

    
    std::print("runMainTask(): About to create the KernelBootTask\n");  
    KernelBootTask kernel_boot_task = mainKernelTask(user_main_task);
    std::print("runMainTask(): Did create the KernelBootTask({0})\n", (void*)&kernel_boot_task.hdl.promise());  
    std::print("runMainTask(): About to run the KernelBootTask({0})\n", (void*)&kernel_boot_task.hdl.promise());      
    kernel_boot_task.run();
    std::print("runMainTask(): did run the KernelBootTask({0})\n", (void*)&kernel_boot_task.hdl.promise());       
        
    finiKernel();
    std::print("runMainTask(): terminted ...\n");  
    return 0; 
}

void initKernel() noexcept {
    g_kernel.task_count = 0;
    g_kernel.ready_count = 0;
    g_kernel.waiting_count = 0;
    g_kernel.io_waiting_count = 0;
    g_kernel.zombie_count = 0;
    g_kernel.interrupted = 0;
    g_kernel.current_task_hdl   = TaskHdl();
    g_kernel.scheduler_task_hdl = TaskHdl();
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

    std::printf(">> SchedulerTask(%p): started\n", &g_kernel.current_task_hdl.promise());
    std::printf(">> SchedulerTask(%p): about to spawn main task\n", &g_kernel.current_task_hdl.promise()); // g_kernel.current_task_promise 
    Task main_task = user_main_task();
    std::printf(">> SchedulerTask(%p): Main task is Task(%p)\n", &g_kernel.current_task_hdl.promise(), &main_task.hdl.promise());
    assert(!main_task.hdl.done());
    assert(main_task.state() == TaskState::READY);
    std::printf(">> SchedulerTask(%p): did spawn main task\n", &g_kernel.current_task_hdl.promise());
    
    debugTaskCount();

    while (true) {
        std::print(">> SchedulerTask({}): begin scheduler loop\n", (void*)&g_kernel.current_task_hdl.promise());

        // If we have a ready task, resume it
        std::print(">> SchedulerTask({}): ready count: {}\n", (void*)&g_kernel.current_task_hdl.promise(), g_kernel.ready_count);
        if (g_kernel.ready_count > 0) {
            std::print(">> SchedulerTask({}): ready count {} > 0\n", (void*)&g_kernel.current_task_hdl.promise(), g_kernel.ready_count);

            // Pop the next ready task and get its handle
            DList* next_ready_promise_node = g_kernel.ready_list.prev;
            TaskPromise* next_ready_promise = waitListNodeToTask(next_ready_promise_node);
            TaskHdl next_ready_task_hdl = TaskHdl::from_promise(*next_ready_promise);
            assert(next_ready_task_hdl != g_kernel.scheduler_task_hdl);
            co_await ExecuteTaskEffect(next_ready_task_hdl);
            continue;
        }
        std::print(">> SchedulerTask({}): ready count == 0\n", (void*)&g_kernel.current_task_hdl.promise());
        DList* zombie_promise_node = g_kernel.zombie_list.pop_front();
        TaskPromise* zombie_promise = waitListNodeToTask(zombie_promise_node);
        TaskHdl zombie_hdl = TaskHdl::from_promise(*zombie_promise);
        zombie_hdl.destroy();
        
        if (g_kernel.zombie_count == 0 && g_kernel.ready_count== 0) break;
    }
    co_return;
}


TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

