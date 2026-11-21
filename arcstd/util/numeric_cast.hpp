#pragma once

#include <boost/numeric/conversion/cast.hpp>

#include <type_traits>

namespace arcstd::util
{

///
/// \return \p v as T, a static_cast in a constant expression, else a boost::numeric_cast
/// \throws boost::numeric::bad_numeric_cast at runtime if T can not hold \p v
///
template<typename T>
[[nodiscard]] constexpr auto numeric_cast_rt(auto&& v) -> T
  requires(std::is_arithmetic_v<T> && std::is_arithmetic_v<std::remove_cvref_t<decltype(v)>>)
{
  if consteval
  {
    return static_cast<T>(std::forward<decltype(v)>(v));
  }
  else
  {
    return boost::numeric_cast<T>(std::forward<decltype(v)>(v));
  }
}

} // namespace arcstd::util
