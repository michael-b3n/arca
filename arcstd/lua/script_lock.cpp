#include "arcstd/lua/script_lock.hpp"

#include <utility>

namespace arcstd::lua
{

///
///
script_lock::script_lock(std::shared_ptr<const void> state)
  : state_{std::move(state)}
{
}

} // namespace arcstd::lua
