#pragma once

#include "ak/alloc/alloc.hpp"         // IWYU pragma: keep
#include "ak/runtime/runtime_api.hpp" // IWYU pragma: keep

AkCoroutineHandle runtime_schedule_next_thread() noexcept;
void            runtime_check_invariants() noexcept;
void            runtime_dump_task_count() noexcept;
void            runtime_dump_io_uring_params(const io_uring_params* p);
void            runtime_dump_alloc_table() noexcept;
void            runtime_dump_alloc_block() noexcept;

