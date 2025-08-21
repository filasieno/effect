// #include "ak/io/io.hpp" // IWYU pragma: keep

// namespace ak {

//     constexpr CThread::Hdl op::ExecIO::await_suspend(CThread::Hdl current_context_hdl) noexcept {
//         // if suspend is called we know that the operation has been submitted
//         using namespace priv;
        

//         // Move the current Task from RUNNING to IO_WAITING
//         auto* current_context = get_context(current_context_hdl);
//         AK_ASSERT(current_context->state == CThread::State::RUNNING);
//         current_context->state = CThread::State::IO_WAITING;
//         ++global_kernel_state.iowaiting_cthread_count;
//         global_kernel_state.current_cthread.reset();
//         check_invariants();
//         dump_task_count();

//         // Move the scheduler task from READY to RUNNING
//         auto* sched_ctx = get_context(global_kernel_state.scheduler_cthread);
//         AK_ASSERT(sched_ctx->state == CThread::State::READY);
//         sched_ctx->state = CThread::State::RUNNING;
//         detach_dlink(&sched_ctx->wait_link);
//         --global_kernel_state.ready_cthread_count;
//         global_kernel_state.current_cthread = global_kernel_state.scheduler_cthread;
//         check_invariants();
//         dump_task_count();

//         return global_kernel_state.scheduler_cthread;
//     }

//     // IO Operations
//     // ----------------------------------------------------------------------------------------------------------------

//     namespace priv {
//         template <typename PrepFn>
//         inline op::ExecIO prep_io(PrepFn prep_fn) noexcept {
//             using namespace priv;
            
//             CThread::Context* ctx = get_context(global_kernel_state.current_cthread);
    
//             // Ensure free submission slot 
//             unsigned int free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
//             while (free_slots < 1) {
//                 int ret = io_uring_submit(&global_kernel_state.io_uring_state);
//                 if (ret < 0) {
//                     abort();
//                     // unreachable
//                 }
//                 free_slots = io_uring_sq_space_left(&global_kernel_state.io_uring_state);
//             }
    
//             // Enqueue operation
//             io_uring_sqe* sqe = io_uring_get_sqe(&global_kernel_state.io_uring_state);
//             io_uring_sqe_set_data(sqe, (Void*) ctx);
//             prep_fn(sqe);  // Call the preparation function
//             ctx->res = 0;
//             ++ctx->prepared_io;
//             return {};
//         }
//     }

// } // namespace ak