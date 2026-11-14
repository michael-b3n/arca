#pragma once

// beman.execution implements std::execution of C++26. Include this header instead of the
// library, the alias below is the one place to change once the standard library ships it.

// The library reads its configuration while being included, every translation unit has to see the same one.
#if defined(INCLUDED_BEMAN_EXECUTION_DETAIL_COMMON)
  #error "beman.execution was included without its configuration, include arcstd/execution/execution.hpp instead"
#endif

// arcstd brings the thread pool of the parallel scheduler itself, \see thread_pool.cpp
#define BEMAN_EXECUTION_WITH_DEFAULT_PARALLEL_SCHEDULER_BACKEND 0

#include <beman/execution/execution.hpp>
#include <beman/execution/stop_token.hpp>

namespace arcstd
{

namespace ex = beman::execution;

} // namespace arcstd
