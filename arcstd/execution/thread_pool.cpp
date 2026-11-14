#include "arcstd/execution/execution.hpp"

#include <beman/execution/detail/thread_pool_backend.hpp>

#include <memory>

namespace beman::execution::parallel_scheduler_replacement
{

///
/// The thread pool of arcstd, the backend of the parallel scheduler in place of the one the library defaults to.
/// It is never destroyed: a shared library destroys its statics once the process ended its threads, joining them
/// would wait forever. Launched work is waited for by its scope.
///
auto query_parallel_scheduler_backend() -> std::shared_ptr<parallel_scheduler_backend>
{
  static const auto* const backend =
    new std::shared_ptr<parallel_scheduler_backend>{std::make_shared<detail::thread_pool_backend>()};
  return *backend;
}

} // namespace beman::execution::parallel_scheduler_replacement
