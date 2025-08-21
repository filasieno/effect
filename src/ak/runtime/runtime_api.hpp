#pragma once

#include <coroutine>
#include <liburing.h>

#include "ak/base/base.hpp"   // IWYU pragma: keep
#include "ak/alloc/alloc.hpp" // IWYU pragma: keep

namespace ak { 
   
 
    /// \brief A handle to a cooperative thread (C++20 coroutine)
    /// \ingroup CThread
    struct CThread {
        struct Context;
        using Hdl = std::coroutine_handle<Context>;
        using promise_type = Context;

        /// \brief Idenfies the state of a task
        /// \ingroup Task
        enum class State
        {
            INVALID = 0, ///< Invalid OR uninitialized state
            CREATED,     ///< Task has been created (BUT NOT REGISTERED WITH THE RUNTINME)
            READY,       ///< Ready for execution
            RUNNING,     ///< Currently running
            IO_WAITING,  ///< Waiting for IO
            WAITING,     ///< Waiting for an event
            ZOMBIE,      ///< Already dead
            DELETING     ///< Currently being deleted
        };

        struct Context {
            using DLink = priv::DLink;
            struct SizeProbe {};
    
            struct InitialSuspendTaskOp {
                constexpr Bool await_ready() const noexcept  { return false; }
                constexpr Void await_resume() const noexcept {}
                Void           await_suspend(Hdl hdl) const noexcept;
            };
    
            struct FinalSuspendTaskOp {
                constexpr Bool await_ready() const noexcept  { return false; }
                constexpr Void await_resume() const noexcept {}
                Hdl            await_suspend(Hdl hdl) const noexcept;
            };
    
            static Void*   operator new(std::size_t n) noexcept;
            static Void    operator delete(Void* ptr, std::size_t sz);
            static CThread get_return_object_on_allocation_failure() noexcept { return {}; }
    
            template <typename... Args>
            Context(Args&&...);
            ~Context();
            
            CThread        get_return_object() noexcept     { return { Hdl::from_promise(*this) }; }
            constexpr auto initial_suspend() const noexcept { return InitialSuspendTaskOp {}; }
            constexpr auto final_suspend () const noexcept  { return FinalSuspendTaskOp{}; }
            Void           return_value(int value) noexcept;
            Void           unhandled_exception() noexcept;

            State state;
            int   res;
            U32   prepared_io;
            DLink wait_link;     //< Used to enqueue tasks waiting for Critical Section
            DLink tasklist_link; //< Global Task list
            DLink awaiter_list;  //< The list of all tasks waiting for this task
        };

        CThread() = default;
        CThread(const CThread&) = default;
        CThread(CThread&&) = default;
        CThread& operator=(const CThread&) = default;
        CThread& operator=(CThread&&) = default;
        ~CThread() = default;

        CThread(const Hdl& hdl) : hdl(hdl) {}
        CThread& operator=(const Hdl& hdl) {
            this->hdl = hdl;
            return *this;
        }    
        Bool operator==(const CThread& other) const noexcept { return hdl == other.hdl; }
        operator Bool() const noexcept { return hdl.address() != nullptr; }
        operator Hdl() const noexcept { return hdl; }
        
        Void reset() noexcept {
            hdl = Hdl{};
        }

        Hdl hdl;
    };
    const Char* to_string(CThread::State state) noexcept;
    
    inline CThread::Hdl to_handle(CThread::Context* cthread_context) noexcept {
        return CThread::Hdl::from_promise(*cthread_context);        
    }


    // Kernel
    // ----------------------------------------------------------------------------------------------------------------
    
    struct BootCThread {
        struct Context {
            using InitialSuspend = std::suspend_always;
            using FinalSuspend   = std::suspend_never;
    
            static Void*       operator new(std::size_t) noexcept;
            static Void        operator delete(Void*, std::size_t) noexcept {};
            static BootCThread
            get_return_object_on_allocation_failure() noexcept;

            template <typename... Args>
            Context(CThread(*)(Args ...) noexcept, Args... ) noexcept : exit_code(0) {}
            
            constexpr BootCThread    get_return_object() noexcept { return {Hdl::from_promise(*this)}; }
            constexpr InitialSuspend initial_suspend() noexcept { return {}; }
            constexpr FinalSuspend   final_suspend() noexcept { return {}; }
            constexpr Void           return_void() noexcept { }
            constexpr Void           unhandled_exception() noexcept;
    
            int exit_code;
        };

        using promise_type = Context;

        using Hdl = std::coroutine_handle<Context>;

        BootCThread() = default;
        BootCThread(const Hdl& hdl) : hdl(hdl) {}
        BootCThread(const BootCThread& other) noexcept = default;
        BootCThread& operator=(const BootCThread& other) = default;
        BootCThread(BootCThread&& other) = default;
        ~BootCThread() = default;

        operator Hdl() const noexcept { return hdl; }

        Hdl hdl;
    };
    
    struct Kernel {
        using DLink = priv::DLink;
        
        // Allocation table
        AllocTable alloc_table;
        
        // Task management
        Char        boot_cthread_frame_buffer[64];
        BootCThread boot_cthread;
        CThread     current_cthread;
        CThread     scheduler_cthread;
        CThread     main_cthread;
        
        DLink       zombie_list;
        DLink       ready_list;
        DLink       cthread_list;
        Void*       mem;
        Size        mem_size; // remove mem begin+end
        I32         main_cthread_exit_code;

        // Count state variables
        I32         cthread_count;
        I32         ready_cthread_count;
        I32         waiting_cthread_count;
        I32         iowaiting_cthread_count;
        I32         zombie_cthread_count;
        I32         interrupted;
        
        // IOManagement
        io_uring    io_uring_state;
        U32         ioentry_count;
    };

    extern Kernel global_kernel_state;
    
    // Main Routine
    struct KernelConfig {
        Void*    mem;
        Size     memSize;
        unsigned ioEntryCount;
    };

    int  init_kernel(KernelConfig* config) noexcept;
    Void fini_kernel() noexcept;

    template <typename... Args>
    int run_main(CThread (*co_main)(Args ...) noexcept, Args... args) noexcept;
    
    struct Event { priv::DLink wait_list; };
    //
    // Declarations for ops 
    namespace op {
        struct ResumeCThread {
            using Hdl = CThread::Hdl;
            explicit ResumeCThread(CThread ct) : hdl(ct.hdl) {};
    
            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
    
            Hdl hdl;
        };

        struct JoinCThread {
            using Hdl = CThread::Hdl;
            explicit JoinCThread(Hdl hdl) : hdl(hdl) {};
    
            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr int  await_resume() const noexcept { return hdl.promise().res; }
    
            Hdl hdl;
        };

        struct Suspend {
            using Hdl = CThread::Hdl;

            constexpr Bool await_ready() const noexcept { return false; }
            Hdl            await_suspend(Hdl hdl) const noexcept;
            constexpr Void await_resume() const noexcept {}
        };

        struct GetCurrentTask {
            using Hdl = CThread::Hdl;
            constexpr Bool await_ready() const noexcept { return false; }
            constexpr Hdl  await_suspend(Hdl hdl) noexcept;
            constexpr Hdl  await_resume() const noexcept { return hdl; }

            Hdl hdl;
        };



        struct WaitEvent {
            explicit WaitEvent(Event* event) : evt(event) {}

            constexpr Bool         await_ready() const noexcept { return false; }
            constexpr CThread::Hdl await_suspend(CThread::Hdl hdl) const noexcept;
            constexpr Void         await_resume() const noexcept {}

            Event* evt;
        };
    }
    // Declarations for ops 

    // CThread routines
    Bool                is_valid(CThread thread) noexcept;
    Bool                is_done(CThread thread) noexcept;
    CThread::Context*   get_context() noexcept;
    CThread::Context*   get_context(CThread thread) noexcept;
    CThread::State      get_state(CThread thread) noexcept;
    op::JoinCThread     join(CThread thread) noexcept;
    op::JoinCThread     operator co_await(CThread thread) noexcept;
    op::ResumeCThread   resume(CThread thread) noexcept;
    
    constexpr op::Suspend suspend() noexcept;

    // Remove
    constexpr op::GetCurrentTask get_cthread_context_async() noexcept; //< Duplicated remove.
    
}