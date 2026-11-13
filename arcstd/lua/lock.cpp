#include "arcstd/lua/lock.hpp"

#include <utility>

namespace arcstd::lua
{

///
///
lock::lock(std::shared_ptr<const void> state)
  : state_{std::move(state)}
{
}

} // namespace arcstd::lua
