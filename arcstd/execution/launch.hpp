#pragma once

#include "arcstd/execution/scope.hpp"

#include <functional>

namespace arcstd::execution
{

///
/// Run \p work on the thread pool, detached in \p scope which waits for it. An exception it throws
/// is logged.
///
auto launch(std::move_only_function<void()> work, scope& scope) -> void;

} // namespace arcstd::execution
