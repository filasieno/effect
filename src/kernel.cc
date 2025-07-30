#include "task.hpp"

struct Kernel gKernel;

static void initKernel() noexcept;
static void finiKernel() noexcept; 

struct KernelBootPromise;

using KernelBootTaskHdl = std::coroutine_handle<KernelBootPromise>;

struct RunSchedulerTaskOp {

    RunSchedulerTaskOp(TaskHdl schedulerHdl) : schedulerHdl(schedulerHdl) {};

    bool await_ready() const noexcept {
        return schedulerHdl.done();
    }

    TaskHdl await_suspend(KernelBootTaskHdl currentTaskHdl) const noexcept {
        (void)currentTaskHdl;
        TaskPromise& scheduler_promise = schedulerHdl.promise();
        std::print("ExecuteSchedulerTaskEffect::await_suspend: about to suspend KernelBootTaskHdl({0})\n", (void*)&currentTaskHdl.promise()); 
        
        // Check expected state post scheduler construction
        
        assert(gKernel.taskCount == 1);
        assert(gKernel.readyCount == 1);
        assert(scheduler_promise.state == TaskState::READY);
        assert(!scheduler_promise.waitNode.detached());
        assert(gKernel.currentTask == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        gKernel.currentTask = schedulerHdl;
        scheduler_promise.state = TaskState::RUNNING;
        scheduler_promise.waitNode.detach();
        --gKernel.readyCount;
    
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

    RunSchedulerTaskOp run() const noexcept {
        return RunSchedulerTaskOp{hdl};
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
    gKernel.schedulerTask = scheduler_task.hdl;
    std::print("mainKernelTask(std::function<Task): did spawn SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise()); 

    std::print("mainKernelTask(std::function<Task): about to execute SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise());  
    co_await scheduler_task.run();
    std::print("mainKernelTask(std::function<Task): did to execute SchedulerTask({0})\n", (void*)&scheduler_task.hdl.promise());  
    
    // The scheduler task never returns; it runs until completion
    assert(scheduler_task.hdl.done());

    std::print("mainKernelTask(std::function<Task): terminated the main kernel task\n"); 
    co_return;
}

int runMainTask(std::function<Task()> userMainTask) noexcept {
    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr); 

    std::print("runMainTask(): started ...\n"); 

    initKernel();

    std::print("runMainTask(): About to create the KernelBootTask\n");  
    KernelBootTask kernelBootTask = mainKernelTask(userMainTask);
    std::print("runMainTask(): Did create the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());  
    std::print("runMainTask(): About to run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());      
    kernelBootTask.run();
    std::print("runMainTask(): did run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());       
        
    finiKernel();

    std::print("runMainTask(): terminted ...\n");  
    return 0; 
}

void initKernel() noexcept {
    gKernel.taskCount = 0;
    gKernel.readyCount = 0;
    gKernel.waitingCount = 0;
    gKernel.ioWaitingCount = 0;
    gKernel.zombieCount = 0;
    gKernel.interrupted = 0;
    gKernel.currentTask.clear();
    gKernel.schedulerTask.clear();
    gKernel.zombieList.init();
    gKernel.readyList.init();
    gKernel.taskList.init();
    std::print("Kernel::init(): initialized\n");

    // Check invariants
    checkTaskCountInvariant();
}

void finiKernel() noexcept {
    // TODO: add checks to ensure all tasks are finalized

    std::print("Kernel::fini(): finalized\n");
    
    // Check invariants
    checkTaskCountInvariant();
}

static int started = 0;

SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept {    
    ++started;
    std::fflush(stdout);
    assert(started == 1);

    std::print(">> SchedulerTask({}): started\n", (void*)&gKernel.currentTask.promise());
    std::print(">> SchedulerTask({}): about to spawn main task\n", (void*)&gKernel.currentTask.promise()); // g_kernel.current_task_promise 
    Task main_task = user_main_task();
    std::print(">> SchedulerTask({}): Main task is Task({})\n", (void*)&gKernel.currentTask.promise(), (void*)&main_task.hdl.promise());
    assert(!main_task.hdl.done());
    assert(main_task.state() == TaskState::READY);
    std::print(">> SchedulerTask({}): did spawn main task\n", (void*)&gKernel.currentTask.promise());
    
    debugTaskCount();

    while (true) {
        std::print(">> SchedulerTask({}): begin scheduler loop\n", (void*)&gKernel.currentTask.promise());

        // If we have a ready task, resume it
        std::print(">> SchedulerTask({}): ready count: {}\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);
        if (gKernel.readyCount > 0) {
            std::print(">> SchedulerTask({}): ready count {} > 0\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);

            // Pop the next ready task and get its handle
            DList* next_ready_promise_node = gKernel.readyList.prev;
            TaskPromise* next_ready_promise = waitListNodeToTaskPromise(next_ready_promise_node);
            TaskHdl next_ready_task_hdl = TaskHdl::from_promise(*next_ready_promise);
            assert(next_ready_task_hdl != gKernel.schedulerTask);
            co_await ResumeTaskOp(next_ready_task_hdl);
            continue;
        }
        std::print(">> SchedulerTask({}): ready count == 0\n", (void*)&gKernel.currentTask.promise());
        
        // Zombie killing
        // TODO: kill zombies
        //
        // DList* zombie_promise_node = gKernel.zombieList.pop_front();
        // TaskPromise* zombie_promise = waitListNodeToTask(zombie_promise_node);
        // TaskHdl zombie_hdl = TaskHdl::from_promise(*zombie_promise);
        // zombie_hdl.destroy();        
        // if (gKernel.zombieCount == 0 && gKernel.readyCount== 0) break;

        if (gKernel.readyCount== 0) break;
    }
    co_return;
}


TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

