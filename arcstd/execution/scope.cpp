#include "arcstd/execution/scope.hpp"
#include "arcstd/execution/scope_data.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/log.hpp"

#include <utility>

namespace arcstd::execution
{

///
///
scope::scope()
  : data_{std::make_unique<detail::scope_data>()}
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
    data_->scope.request_stop();
    ex::sync_wait(data_->scope.join());
  }
  catch(...)
  {
    LOG_ERROR("scope exception: {}", util::exception_report());
  }
}

} // namespace arcstd::execution
