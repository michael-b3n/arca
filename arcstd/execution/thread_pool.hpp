#pragma once

#include "arcstd/export.hpp"

namespace arcstd::execution
{

///
/// Destroy the thread pool and join its threads. The app calls it last, once the objects that
/// launch work are gone. A pool still running work is kept and logged, later work starts a new
/// pool. Without the call the pool is never destroyed: a shared arcstd destroys its statics after
/// Windows ended the threads, joining them then would never return.
///
ARCSTD_EXPORT auto shutdown_thread_pool() noexcept -> void;

} // namespace arcstd::execution
