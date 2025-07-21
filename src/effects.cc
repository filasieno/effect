#include <coroutine>
#include <print>

struct TaskPromise;

struct Task;

static Task* g_scheduler;
static Task* g_current;

struct TaskSystem {
    static auto init(Task& s) -> void {
        g_scheduler = &s;
        g_current   = &s;
    }
};

typedef std::coroutine_handle<TaskPromise> TaskHdl;
typedef std::coroutine_handle<> CoroHdl;

struct Task {
    using promise_type = TaskPromise;
    Task() = default;
    Task(TaskHdl hdl) : hdl(hdl) {}
    void resume() {
        if (!hdl.done()) hdl.resume();
    }
    void destroy() {
        hdl.destroy();
    }
    TaskHdl hdl;
};

struct SuspendEffect {
    constexpr bool await_ready() const noexcept { 
        std::printf("Suspend effect:is_ready?\n");
        return false; 
    }
    constexpr void await_suspend(CoroHdl hdl) const noexcept {
        std::printf("Suspend effect:await suspend\n");        
    }
    constexpr void await_resume() const noexcept {
        std::printf("Suspend effect:about to resume\n");
    }
};

struct NopEffect {
    constexpr bool await_ready() const noexcept { 
        std::printf("Suspend effect:is_ready?\n");
        return false;
    }
    constexpr void await_suspend(CoroHdl hdl) const noexcept {}
    constexpr void await_resume() const noexcept {}
};

constexpr auto suspend() -> SuspendEffect {
    return {};
}

constexpr auto nop() -> NopEffect {
    return {};
}

struct TaskPromise {
    ~TaskPromise() {
        std::printf("Finalized promise\n");
    }

    void* operator new(std::size_t n) noexcept
    {
        printf("Allocating a promise of: %zu\n", n);
        void* mem = std::malloc(n);
        if (!mem)
            return nullptr;
        return mem;
    }

    TaskPromise() {
        std::println("Promise initialized");
    }
    
    Task get_return_object() noexcept { 
        std::println("get return object");
        return Task {TaskHdl::from_promise(*this) };
    }

    SuspendEffect initial_suspend() noexcept { 
        std::println("initial suspend");
        return {}; 
    }
    SuspendEffect final_suspend() noexcept { 
        std::println("final suspend");
        return {}; 
    }
    void return_void() noexcept {
        std::println("returning void");
    }
    void unhandled_exception() noexcept {
        std::println("excaption raised");
    }

    void operator delete(void* ptr, std::size_t sz) {
        std::println("Release Promise memory");
        std::free(ptr);
    }
    
};

auto coro() -> Task  {
    std::println("Step 1...");
    co_await suspend();
    std::println("Step 2...");
    co_await suspend();
    std::println("Step 3...");
    co_return;
}

auto scheduler() -> Task {
    while (true) {
        co_await suspend();
    }
    co_return;
}

int main() {

    auto scheduler_task = scheduler();
    TaskSystem::init(scheduler_task);

    auto x = coro();
    std::println("resumed....");
    x.resume();
    std::println("resumed....");
    x.resume();
    std::println("resumed....");
    x.resume();
    std::println("resumed....");
    x.resume();

    scheduler_task.destroy();
    x.destroy();
    
};
