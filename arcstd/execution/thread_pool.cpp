#include "arcstd/execution/thread_pool.hpp"
#include "arcstd/execution/execution.hpp"
#include "arcstd/util/log.hpp"
#include "arcstd/util/no_destructor.hpp"

#include <beman/execution/detail/thread_pool_backend.hpp>

#include <memory>
#include <mutex>
#include <utility>

namespace arcstd::execution
{
namespace
{

using backend_type = beman::execution::parallel_scheduler_replacement::parallel_scheduler_backend;

///
/// The thread pool of the parallel scheduler, created by the first launch.
///
struct thread_pool final
{
  std::mutex mtx;
  std::shared_ptr<backend_type> backend;
};

///
/// \return the thread pool, never destroyed: its destructor would run too late, \see shutdown_thread_pool
///
[[nodiscard]] auto pool() -> thread_pool&
{
  static auto instance = util::no_destructor<thread_pool>{};
  return *instance;
}

} // namespace

///
///
auto shutdown_thread_pool() noexcept -> void
{
  auto& p = pool();
  auto backend = std::shared_ptr<backend_type>{};
  {
    const auto lock = std::scoped_lock{p.mtx};
    // Work in flight holds the pool too, the last one to let go would join its own thread
    if(p.backend.use_count() > 1)
    {
      LOG_WARN("thread pool still runs work, it is kept");
      return;
    }
    backend = std::exchange(p.backend, nullptr);
  }
  // Destroyed here, outside the lock, it joins its threads
}

} // namespace arcstd::execution

namespace beman::execution::parallel_scheduler_replacement
{

///
/// The backend of the parallel scheduler, in place of the one the library defaults to. That one is a static of an
/// inline function: no app can shut it down, and a shared arcstd destroys it after Windows ended its threads, the join
/// then never returns. The library offers this function to be replaced, std::execution does the same.
///
auto query_parallel_scheduler_backend() -> std::shared_ptr<parallel_scheduler_backend>
{
  auto& p = arcstd::execution::pool();
  const auto lock = std::scoped_lock{p.mtx};
  if(!p.backend)
  {
    p.backend = std::make_shared<detail::thread_pool_backend>();
  }
  return p.backend;
}

} // namespace beman::execution::parallel_scheduler_replacement
