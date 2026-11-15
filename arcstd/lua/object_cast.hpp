#pragma once

#ifndef ARCSTD_INTERNAL
  #error "private header of arcstd, a project includes the public ones only"
#endif

#include "arcstd/lua/function_cast.hpp"
#include "arcstd/lua/sol.hpp"
#include "arcstd/lua/value.hpp"

#include <functional>

namespace arcstd::lua
{

///
/// The Lua function calling \p function, to set in a Lua table. The arguments of the call and the results are copied.
///
using object_function = std::function<sol::variadic_results(sol::this_state, sol::variadic_args)>;

///
/// \return \p v as object of \p lua, an opaque value is nil
///
[[nodiscard]] auto to_object(sol::state_view lua, const value& v) -> sol::object;

///
/// \return a copy of \p object. A table nested deeper than any C++ type takes is opaque, so is one containing itself.
///
[[nodiscard]] auto to_value(const sol::object& object) -> value;

///
/// \return the Lua function calling \p function
///
[[nodiscard]] auto to_object_function(function function) -> object_function;

} // namespace arcstd::lua
