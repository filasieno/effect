#include "task.hpp"

struct Kernel g_kernel;

static void initKernel() noexcept;
static void finiKernel() noexcept; 
static SchedulerTask scheduler(std::function<Task()> user_main_task) noexcept;

int startMainTask(std::function<Task()> user_main_task) noexcept {
    initKernel();
        
    SchedulerTask scheduler_task = scheduler(user_main_task);
    std::printf("startMainTask(std::function<Task()>): Scheduler is Task(%p)\n", &scheduler_task.hdl.promise());
    g_kernel.scheduler_task_promise = &scheduler_task.hdl.promise();

    // Check expected state post scheduler construction
    assert(g_kernel.current_task_promise == nullptr);
    assert(g_kernel.task_count == 1);
    assert(g_kernel.ready_count == 1);
    assert(scheduler_task.state() == TaskState::READY);
    assert(!scheduler_task.hdl.done());

    // bootstrap task system
    g_kernel.current_task_promise = &scheduler_task.hdl.promise();
    g_kernel.current_task_promise->state = TaskState::RUNNING;
    g_kernel.current_task_promise->wait_node.detach();
    --g_kernel.ready_count;
    
    // Check expected state post task system bootstrap
    checkTaskCountInvariant();
    
    // Initialize here the scheduler task state 
    scheduler_task.resume(); 
    
    // The scheduler task never returns; it runs until completion
    assert(scheduler_task.hdl.done());
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
    std::printf(">> started: %d\n", started);
    std::fflush(stdout);
    assert(started == 1);
    std::printf(">> SchedulerTask(%p): started\n", g_kernel.current_task_promise);
    std::printf(">> SchedulerTask(%p): about to spawn main task\n", g_kernel.current_task_promise); // g_kernel.current_task_promise 
    Task main_task = user_main_task();
    std::printf(">> SchedulerTask(%p): Main task is Task(%p)\n", g_kernel.current_task_promise, &main_task.hdl.promise());
    assert(!main_task.hdl.done());
    assert(main_task.state() == TaskState::READY);
    std::printf(">> SchedulerTask(%p): did spawn main task\n", g_kernel.current_task_promise);

    main_task.resume();

    std::printf("========= Entering =======\n");
    std::fflush(stdout);            
    assert(false);


    while (true) {
        // If we have a ready task, resume it
        if (g_kernel.ready_count > 0) {
            DList* next_ready_promise_node = g_kernel.ready_list.pop_front();
            TaskPromise* next_ready_promise = wait_node_to_promise(next_ready_promise_node);

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


            std::printf("========= ALL OK : HALING =======\n");
            std::fflush(stdout);            
            assert(false);
        }
    }
    co_return;
}


