#pragma once

#include "arcstd/math/arithmetic.hpp"

#include <magic_enum/magic_enum.hpp>

#include <string>
#include <type_traits>

namespace arcstd::util
{

template<typename E>
concept enum_type = std::is_enum_v<E>;

///
/// \return the underlying value of \p e
///
template<enum_type E>
[[nodiscard]] constexpr auto to_integral(const E e) -> std::underlying_type_t<E>
{
  return static_cast<std::underlying_type_t<E>>(e);
}

///
/// \return the name of \p e, empty if it names no value
///
template<enum_type E>
[[nodiscard]] constexpr auto enum_name(const E e) -> std::string_view
{
  return magic_enum::enum_name(e);
}

///
/// \return the number of named values of \p E
///
template<enum_type E>
[[nodiscard]] consteval auto enum_count() -> std::size_t
{
  return magic_enum::enum_count<E>();
}

///
/// \return the names of the values of \p E, in their order
///
template<enum_type E>
[[nodiscard]] consteval auto enum_names() -> std::array<std::string_view, enum_count<E>()>
{
  return magic_enum::enum_names<E>();
}

///
/// \return the named values of \p E, in their order
///
template<enum_type E>
[[nodiscard]] consteval auto enum_values() -> std::array<E, enum_count<E>()>
{
  return magic_enum::enum_values<E>();
}

///
/// \return the value of \p integral, named or not
///
template<enum_type E>
[[nodiscard]] constexpr auto to_enum(const std::underlying_type_t<E> integral) -> E
{
  return static_cast<E>(integral);
}

///
/// \return the value named \p name, std::nullopt if there is none
///
template<enum_type E>
[[nodiscard]] constexpr auto to_enum(const std::string_view name) -> std::optional<E>
{
  return magic_enum::enum_cast<E>(name);
}

///
/// \return the value after \p e, \see has_next
/// \throws util::exception if the underlying type overflows
///
template<enum_type E>
[[nodiscard]] constexpr auto next(const E e) -> E
{
  const auto value = math::arithmetic::add(to_integral(e), std::underlying_type_t<E>(1));
  if(!value.has_value())
  {
    throw util::exception("invalid next enum value");
  }
  return to_enum<E>(*value);
}

///
/// \return true if the value after \p e is named
///
template<enum_type E>
[[nodiscard]] constexpr auto has_next(const E e) -> bool
{
  const auto value = math::arithmetic::add(to_integral(e), std::underlying_type_t<E>(1));
  return value.has_value() && magic_enum::enum_cast<E>(*value).has_value();
}

///
/// \return the value before \p e, \see has_prev
/// \throws util::exception if the underlying type overflows
///
template<enum_type E>
[[nodiscard]] constexpr auto prev(const E e) -> E
{
  const auto value = math::arithmetic::subtract(to_integral(e), std::underlying_type_t<E>(1));
  if(!value.has_value())
  {
    throw util::exception("invalid previous enum value");
  }
  return to_enum<E>(*value);
}

///
/// \return true if the value before \p e is named
///
template<enum_type E>
[[nodiscard]] constexpr auto has_prev(const E e) -> bool
{
  const auto value = math::arithmetic::subtract(to_integral(e), std::underlying_type_t<E>(1));
  return value.has_value() && magic_enum::enum_cast<E>(*value).has_value();
}

///
/// \return true if \p e is a named value
///
template<enum_type E>
[[nodiscard]] constexpr auto valid(E e) -> bool
{
  return magic_enum::enum_cast<E>(to_integral(e)).has_value();
}

} // namespace arcstd::util
