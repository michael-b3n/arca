#pragma once

#include "arcstd/execution/execution.hpp"

namespace arcstd::execution::detail
{

///
/// What a scope keeps of the execution library, out of its header.
///
struct scope_data final
{
  ex::counting_scope scope;
};

} // namespace arcstd::execution::detail
