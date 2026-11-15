#include "arcstd/execution/scope.hpp"
#include "arcstd/execution/execution.hpp"
#include "arcstd/execution/launch.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/log.hpp"

#include <exception>
#include <utility>

namespace arcstd::execution
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
/// What the scope keeps of the execution library.
///
struct scope::impl final
{
  ex::counting_scope scope;
};

///
///
auto launch(std::move_only_function<void()> work, scope& scope) -> void
{
  ex::spawn(
    ex::schedule(ex::get_parallel_scheduler()) | ex::then(std::move(work)) | ex::upon_error(&log_launch_error),
    scope.impl_->scope.get_token()
  );
}

///
///
scope::scope()
  : impl_{std::make_unique<impl>()}
{
}

///
///
scope::~scope() noexcept
{
  stop();
}

///
///
auto scope::stop() noexcept -> void
{
  const auto lock = std::scoped_lock{mtx_};
  if(std::exchange(stopped_, true))
  {
    return;
  }
  try
  {
    impl_->scope.request_stop();
    ex::sync_wait(impl_->scope.join());
  }
  catch(...)
  {
    LOG_ERROR("scope exception: {}", util::exception_report());
  }
}

} // namespace arcstd::execution
