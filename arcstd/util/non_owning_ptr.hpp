#pragma once

#include <type_traits>

namespace arcstd::util
{

///
/// Pointer that owns nothing, its target lives elsewhere.
///
template<typename T>
  requires(!std::is_pointer_v<T>)
using non_owning_ptr = T*;

} // namespace arcstd::util
