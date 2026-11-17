#pragma once

#include "arcstd/meta/contains.hpp"
#include "arcstd/meta/for_each.hpp"
#include "arcstd/util/enum.hpp"
#include "arcstd/util/exception.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace arcstd::setting
{
namespace detail
{

using setting_basic_variant = std::variant<
  // bool is added on its own, a std::vector<bool> is no container of bools
  std::int32_t,
  std::int64_t,
  std::uint32_t,
  std::uint64_t,
  double,
  std::string,
  std::chrono::milliseconds,
  std::chrono::seconds,
  std::chrono::minutes,
  std::filesystem::path>;

using setting_basic_optional_variant = meta::for_each_t<setting_basic_variant, std::optional>;

using setting_basic_list_variant = meta::for_each_t<setting_basic_variant, std::vector>;

///
/// An enum setting is erased to the name of its value.
///
template<typename E>
  requires(std::is_enum_v<E>)
[[nodiscard]] auto to_type_erased_setting(const E& v) -> std::string
{
  return std::string{util::enum_name(v)};
}

///
/// An optional enum setting is erased to the name of its value, if it holds one.
///
template<typename E>
  requires(std::is_enum_v<E>)
[[nodiscard]] auto to_type_erased_setting(const std::optional<E>& v) -> std::optional<std::string>
{
  return v ? std::optional{to_type_erased_setting(*v)} : std::nullopt;
}

///
/// A list of enum settings is erased to the names of its values.
///
template<typename E>
  requires(std::is_enum_v<E>)
[[nodiscard]] auto to_type_erased_setting(const std::vector<E>& v) -> auto
{
  return v | std::views::transform([](const auto& item) { return to_type_erased_setting(item); }) |
         std::ranges::to<std::vector<std::string>>();
}

///
/// \throws util::exception if \p v names no value of the enum
///
template<typename E>
  requires(std::is_enum_v<E>)
[[nodiscard]] auto from_type_erased_setting(const std::string& v) -> E
{
  const auto value = util::to_enum<E>(v);
  if(!value)
  {
    throw util::exception(std::format("invalid enum setting value: value=\"{}\"", v));
  }
  return *value;
}

template<typename T>
  requires(
    std::is_same_v<T, std::optional<typename T::value_type>> &&
    std::is_enum_v<typename T::value_type>
  )

///
/// \throws util::exception if \p v names no value of the enum
///
[[nodiscard]] auto from_type_erased_setting(const std::optional<std::string>& v) -> T
{
  return v ? T{from_type_erased_setting<typename T::value_type>(*v)} : T{};
}

template<typename T>
  requires(
    std::is_same_v<T, std::vector<typename T::value_type>> && std::is_enum_v<typename T::value_type>
  )

///
/// \throws util::exception if one of \p v names no value of the enum
///
[[nodiscard]] auto from_type_erased_setting(const std::vector<std::string>& v) -> T
{
  return v |
         std::views::transform([](const auto& item)
                               { return from_type_erased_setting<typename T::value_type>(item); }) |
         std::ranges::to<T>();
}

///
/// \return std::nullopt if \p v names no value of the enum
///
template<typename E>
  requires(std::is_enum_v<E>)
[[nodiscard]] auto try_from_type_erased_setting(const std::string& v) -> std::optional<E>
{
  return util::to_enum<E>(v);
}

///
/// \return std::nullopt if \p v names no value of the enum
///
template<typename T>
  requires(
    std::is_same_v<T, std::optional<typename T::value_type>> &&
    std::is_enum_v<typename T::value_type>
  )
[[nodiscard]] auto try_from_type_erased_setting(const std::optional<std::string>& v)
  -> std::optional<T>
{
  if(!v)
  {
    return T{};
  }
  return util::to_enum<typename T::value_type>(*v).transform([](const auto value)
                                                             { return T{value}; });
}

///
/// \return std::nullopt if one of \p v names no value of the enum
///
template<typename T>
  requires(
    std::is_same_v<T, std::vector<typename T::value_type>> && std::is_enum_v<typename T::value_type>
  )
[[nodiscard]] auto try_from_type_erased_setting(const std::vector<std::string>& v)
  -> std::optional<T>
{
  const auto values =
    v |
    std::views::transform([](const auto& item)
                          { return util::to_enum<typename T::value_type>(item); }) |
    std::ranges::to<std::vector>();
  if(!std::ranges::all_of(values, [](const auto& value) { return value.has_value(); }))
  {
    return std::nullopt;
  }
  return values | std::views::transform([](const auto& value) { return *value; }) |
         std::ranges::to<T>();
}

template<typename E>
concept type_erasable = requires(E e) {
  { to_type_erased_setting(e) };
};

} // namespace detail

///
/// The types a setting has in the settings file and in its type erased view.
///
using setting_type_erased_variant = meta::combine_pack_t<
  std::variant<bool, std::optional<bool>>,
  detail::setting_basic_variant,
  detail::setting_basic_optional_variant,
  detail::setting_basic_list_variant>;

///
/// A type of setting_type_erased_variant.
///
template<typename T>
concept underlying_setting_type_erased_type = meta::contains_v<setting_type_erased_variant, T>;

///
/// A type a setting can have: a type erased one or an enum, also in an optional or a vector.
///
template<typename T>
concept underlying_setting_type =
  underlying_setting_type_erased_type<T> || detail::type_erasable<T>;

namespace detail
{

// An erased type is its own erased type, setting_type_erased_type_from relies on it
template<underlying_setting_type_erased_type E>
[[nodiscard]] auto to_type_erased_setting(const E& v) -> E
{
  return v;
}

} // namespace detail

///
/// The type a setting of \p T has in the settings file.
///
template<underlying_setting_type T>
using setting_type_erased_type_from = std::conditional_t<
  underlying_setting_type_erased_type<T>,
  T,
  decltype(detail::to_type_erased_setting(std::declval<T>()))>;

///
/// Convert the type erased setting value \p v, as the settings file holds it.
/// \return \p v as T, std::nullopt if it names no value of the enum of T
///
template<underlying_setting_type T>
[[nodiscard]] auto try_from_type_erased_setting(const setting_type_erased_type_from<T>& v)
  -> std::optional<T>
{
  if constexpr(underlying_setting_type_erased_type<T>)
  {
    return v;
  }
  else
  {
    return detail::try_from_type_erased_setting<T>(v);
  }
}

///
/// \return the converter from \p F to \p T, one is the other's type erased type
///
template<underlying_setting_type F, underlying_setting_type T>
  requires(
    std::is_same_v<setting_type_erased_type_from<F>, T> ||
    std::is_same_v<F, setting_type_erased_type_from<T>>
  )
[[nodiscard]] constexpr auto create_setting_value_converter() -> auto
{
  if constexpr(std::is_same_v<F, T>)
  {
    return [](const F& v) -> T { return v; };
  }
  else if constexpr(std::is_same_v<setting_type_erased_type_from<F>, T>)
  {
    return [](const F& v) -> T { return detail::to_type_erased_setting(v); };
  }
  else if constexpr(std::is_same_v<F, setting_type_erased_type_from<T>>)
  {
    return [](const F& v) -> T { return detail::from_type_erased_setting<T>(v); };
  }
}

} // namespace arcstd::setting
