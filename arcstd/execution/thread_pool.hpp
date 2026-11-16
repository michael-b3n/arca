#pragma once

#include "arcstd/export.hpp"

namespace arcstd::execution
{

///
/// Destroy the thread pool launched work runs on, joining its threads. Called by the app before it
/// ends, once the objects that launch work are gone. A pool still running work is kept for later
/// work and the call logs it. Work launched after a shutdown starts a new pool. Without the call
/// the pool lives until the process ends: a shared arcstd destroys its statics only after Windows
/// ended the threads, joining them then would never return.
///
ARCSTD_EXPORT auto shutdown_thread_pool() noexcept -> void;

} // namespace arcstd::execution
