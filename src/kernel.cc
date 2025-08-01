#include "task.hpp"

struct Kernel gKernel;

static void initKernel() noexcept;
static void finiKernel() noexcept; 

struct RunSchedulerTaskOp {

    RunSchedulerTaskOp(TaskHdl schedulerHdl) : schedulerHdl(schedulerHdl) {};

    bool await_ready() const noexcept {
        return schedulerHdl.done();
    }

    TaskHdl await_suspend(KernelBootTaskHdl currentTaskHdl) const noexcept {
        (void)currentTaskHdl;
        TaskPromise& schedulerPromise = schedulerHdl.promise();
        std::print("RunSchedulerTaskOp::await_suspend: about to suspend KernelBootTaskHdl({0})\n", (void*)&currentTaskHdl.promise()); 
        
        // Check expected state post scheduler construction
        
        assert(gKernel.taskCount == 1);
        assert(gKernel.readyCount == 1);
        assert(schedulerPromise.state == TaskState::READY);
        assert(!schedulerPromise.waitNode.detached());
        assert(gKernel.currentTask == TaskHdl());

        // Setup SchedulerTask for execution (from READY -> RUNNING)
        gKernel.currentTask = schedulerHdl;
        schedulerPromise.state = TaskState::RUNNING;
        schedulerPromise.waitNode.detach();
        --gKernel.readyCount;
    
        // Check expected state post task system bootstrap
        checkInvariants();
        std::print("RunSchedulerTaskOp::await_suspend: about to resume Task({0})\n", (void*)&schedulerHdl.promise());
        return schedulerHdl;
    }

    void await_resume() const noexcept {
        std::print("RunSchedulerTaskOp: await_resume\n");
    }

    TaskHdl schedulerHdl;
};

struct TerminateSchedulerOp {
    constexpr bool await_ready() const noexcept { return false; }
    KernelBootTaskHdl await_suspend(TaskHdl hdl) const noexcept { 
        std::print(">> TerminateSchedulerOp: about to terminate the scheduler task\n");         
        assert(gKernel.currentTask == gKernel.schedulerTask);
        assert(gKernel.currentTask == hdl);

        TaskPromise& schedulerPromise = gKernel.schedulerTask.promise();
        assert(schedulerPromise.state == TaskState::RUNNING);
        assert(schedulerPromise.waitNode.detached()); 
        
        schedulerPromise.state = TaskState::ZOMBIE;
        gKernel.currentTask.clear();
        gKernel.zombieList.pushBack(&schedulerPromise.waitNode);
        ++gKernel.zombieCount;
        
        std::print(">> TerminateSchedulerOp: did terminate the scheduler task\n");         
        return gKernel.kernelTask;
    }
    constexpr void await_resume() const noexcept {}
};

struct SchedulerTask {
    using promise_type = TaskPromise;
    
    SchedulerTask(TaskHdl hdl) noexcept : hdl(hdl) {}
    ~SchedulerTask() noexcept { 
        hdl.destroy();
    }
    bool done() const noexcept { return hdl.done(); }
    RunSchedulerTaskOp run() const noexcept { return RunSchedulerTaskOp{hdl}; }
    TaskState state() const noexcept;

    TaskHdl hdl;
};

struct KernelBootPromise {

    KernelBootPromise(std::function<Task()> userMainTask) : userMainTask(userMainTask){
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
        std::print("KernelBootPromise({0})::initial_suspend()\n", (void*)this);    
        return {}; 
    }

    std::suspend_never final_suspend() noexcept { 
        std::print("KernelBootPromise({0})::final_suspend()\n", (void*)this);    
        return {}; 
    }

    void return_void() noexcept {
        std::print("KernelBootPromise({0})::return_void(): Kernel Task has returned\n", (void*)this);    
    }

    void unhandled_exception() noexcept { assert(false); }

    std::function<Task()> userMainTask;
};

struct KernelBootTask {
    using promise_type = KernelBootPromise;
    
    KernelBootTask(KernelBootTaskHdl hdl) noexcept : hdl(hdl) {}
    ~KernelBootTask() noexcept { 
        // hdl.destroy();
    }
    void run() { hdl.resume();}

    KernelBootTaskHdl hdl;
};

static KernelBootTask mainKernelTask(std::function<Task()> userMainTask) noexcept;
static SchedulerTask  schedulerTask(std::function<Task()> userMainTask) noexcept;

int runMainTask(std::function<Task()> userMainTask) noexcept {
    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr); 

    std::print("runMainTask(): started ...\n"); 

    initKernel();

    std::print("runMainTask(): About to create the KernelBootTask\n");  
    KernelBootTask kernelBootTask = mainKernelTask(userMainTask);
    gKernel.kernelTask = kernelBootTask.hdl;
    std::print("runMainTask(): Did create the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());  
    std::print("runMainTask(): About to run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());      
    kernelBootTask.run();
    std::print("runMainTask(): did run the KernelBootTask({0})\n", (void*)&kernelBootTask.hdl.promise());       
        
    finiKernel();

    std::print("runMainTask(): terminated\n");  
    return 0; 
}

KernelBootTask mainKernelTask(std::function<Task()> userMainTask) noexcept {
    
    std::print("mainKernelTask(std::function<Task): started the main kernel task\n"); 
    
    // Spawn the SchedulerTask
    std::print("mainKernelTask(std::function<Task): about to spawn the SchedulerTask\n"); 
    SchedulerTask task = schedulerTask(userMainTask);
    gKernel.schedulerTask = task.hdl;
    std::print("mainKernelTask(std::function<Task): did spawn SchedulerTask({0})\n", (void*)&task.hdl.promise()); 

    std::print("mainKernelTask(std::function<Task): about to execute SchedulerTask({0})\n", (void*)&task.hdl.promise());  
    co_await task.run();
    std::print("mainKernelTask(std::function<Task): did to execute SchedulerTask({0})\n", (void*)&task.hdl.promise());  
    debugTaskCount();
    
    std::print("mainKernelTask(std::function<Task): terminated the main kernel task\n"); 
    co_return;
}

static int started = 0;

SchedulerTask schedulerTask(std::function<Task()> userMainTask) noexcept {    
    ++started;
    std::fflush(stdout);
    assert(started == 1);

    std::print(">> SchedulerTask({}): started\n", (void*)&gKernel.currentTask.promise());
    std::print(">> SchedulerTask({}): about to spawn main task\n", (void*)&gKernel.currentTask.promise()); // g_kernel.current_task_promise 
    Task mainTask = userMainTask();
    std::print(">> SchedulerTask({}): Main task is Task({})\n", (void*)&gKernel.currentTask.promise(), (void*)&mainTask.hdl.promise());
    assert(!mainTask.hdl.done());
    assert(mainTask.state() == TaskState::READY);
    std::print(">> SchedulerTask({}): did spawn main task\n", (void*)&gKernel.currentTask.promise());
    
    debugTaskCount();

    while (true) {
        std::print(">> SchedulerTask({}): begin scheduler loop\n", (void*)&gKernel.currentTask.promise());

        // If we have a ready task, resume it
        std::print(">> SchedulerTask({}): ready count: {}\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);
        if (gKernel.readyCount > 0) {
            std::print(">> SchedulerTask({}): ready count {} > 0\n", (void*)&gKernel.currentTask.promise(), gKernel.readyCount);
            DList* nextNode = gKernel.readyList.prev;
            TaskPromise& nextPromise = *waitListNodeToTaskPromise(nextNode);
            Task nextTask = TaskHdl::from_promise(nextPromise);
            assert(nextTask != gKernel.schedulerTask);
            co_await ResumeTaskOp(nextTask);
            assert(gKernel.currentTask.isValid());
            continue;
        }
        std::print(">> SchedulerTask({}): ready count: 0; zombie count: {}\n", (void*)&gKernel.currentTask.promise(), gKernel.zombieCount);
        
        // Zombie killing
        // while (gKernel.zombieCount > 0) {
        //     std::print(">> SchedulerTask({}): about to delete a zombie (remaining zombies: {})\n", (void*)&gKernel.currentTask.promise(), gKernel.zombieCount); 
        //     DList* zombieNode = gKernel.zombieList.popFront();
        //     TaskPromise& zombiePromise = *waitListNodeToTaskPromise(zombieNode);
        //     TaskHdl zombieTaskHdl = TaskHdl::from_promise(zombiePromise);
        //     zombieTaskHdl.destroy();
        //     --gKernel.zombieCount;
        //     std::print(">> SchedulerTask({}): did delete a zombie\n", (void*)&gKernel.currentTask.promise()); 
        // }
        
        if (gKernel.readyCount == 0) break;
    }
    std::print(">> SchedulerTask({}): scheduler task terminated\n", (void*)&gKernel.currentTask.promise());
    co_await TerminateSchedulerOp {};
    
    assert(false); // Unreachale
    co_return;
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
}

void finiKernel() noexcept {
    // TODO: add checks to ensure all tasks are finalized

    std::print("Kernel::fini(): finalized\n");
}



TaskState SchedulerTask::state() const noexcept {
    return hdl.promise().state;
}

