#pragma once

#include "arcstd/execution/scope.hpp"

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace arcstd::execution
{

///
/// Run \p work on the thread pool, detached in \p scope which waits for it. An exception it throws is logged.
///
template<std::invocable F>
  requires std::is_void_v<std::invoke_result_t<F>>
auto launch(F&& work, scope& scope) -> void
{
  detail::launch(std::move_only_function<void()>{std::forward<F>(work)}, scope);
}

} // namespace arcstd::execution
