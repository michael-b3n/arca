#pragma once

#ifndef ARCSTD_INTERNAL
  #error "private header of arcstd, a project includes the public ones only"
#endif

#include "arcstd/lua/function.hpp"
#include "arcstd/lua/sol.hpp"
#include "arcstd/lua/value.hpp"

#include <functional>

namespace arcstd::lua
{

///
/// What sol calls when a script calls a function: it takes the arguments as sol has them and returns the results.
///
using sol_function = std::function<sol::variadic_results(sol::this_state, sol::variadic_args)>;

///
/// \return \p v as object of \p lua, unsupported is nil
///
[[nodiscard]] auto to_sol(sol::state_view lua, const value& v) -> sol::object;

///
/// \return a copy of \p object. A table nested deeper than any C++ type takes is unsupported, so is one containing itself.
///
[[nodiscard]] auto from_sol(const sol::object& object) -> value;

///
/// \return \p function as sol calls it, the arguments and the results are copied
///
[[nodiscard]] auto to_sol(function function) -> sol_function;

} // namespace arcstd::lua
