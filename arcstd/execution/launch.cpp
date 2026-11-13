#include "arcstd/execution/launch.hpp"
#include "arcstd/execution/execution.hpp"
#include "arcstd/execution/scope_data.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/log.hpp"

#include <exception>

namespace arcstd::execution::detail
{
namespace
{

///
/// Log the error launched work ended with, nobody else is left to receive it.
///
auto log_launch_error(const std::exception_ptr& error) noexcept -> void
{
  try
  {
    std::rethrow_exception(error);
  }
  catch(...)
  {
    LOG_ERROR("launched work failed: {}", util::exception_report());
  }
}

} // namespace

///
/// Compiled here, so the library and the project built on it share the one thread pool.
///
auto launch(std::move_only_function<void()> work, scope& scope) -> void
{
  ex::spawn(
    ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::move(work)) | ex::upon_error(&log_launch_error),
    scope.data_->scope.get_token()
  );
}

} // namespace arcstd::execution::detail
