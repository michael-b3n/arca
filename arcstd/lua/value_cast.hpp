#pragma once

#include "arcstd/lua/value.hpp"
#include "arcstd/meta/chrono.hpp"
#include "arcstd/util/numeric_cast.hpp"

#include <algorithm>
#include <concepts>
#include <filesystem>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::lua
{

///
/// How the C++ type \p T converts to the Lua value a script sees and back. A specialization offers
///   static auto to(const T& v) -> value;
///   static auto from(const value& v) -> std::optional<T>; // std::nullopt if v is no T
/// Both copy. Only plain types convert: no references, pointers, views or const types.
///
template<typename T>
struct value_cast final
{};

namespace detail
{

///
/// A type of its own, no reference and neither const nor volatile.
///
template<typename T>
concept plain = std::same_as<T, std::remove_cvref_t<T>>;

} // namespace detail

///
/// Types that convert to a value and back, \see value_cast.
///
template<typename T>
concept value_castable = detail::plain<T> && requires(const T& v, const value& converted) {
  { value_cast<T>::to(v) } -> std::same_as<value>;
  { value_cast<T>::from(converted) } -> std::same_as<std::optional<T>>;
};

///
/// \return \p v as the value a script sees
///
template<value_castable T>
[[nodiscard]] constexpr auto to_value(const T& v) -> value;

///
/// \return \p v as T, std::nullopt if it is no T
///
template<value_castable T>
[[nodiscard]] constexpr auto from_value(const value& v) -> std::optional<T>;

namespace detail
{

///
/// \return the values of \p items, std::nullopt if one of them is none
///
template<std::ranges::input_range R>
[[nodiscard]] constexpr auto all_or_none(R&& items)
  -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>;

} // namespace detail

///
/// A Lua boolean.
///
template<>
struct value_cast<bool> final
{
  [[nodiscard]] static constexpr auto to(bool v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<bool>;
};

///
/// A Lua integer, a number is none even of an integral value. Out of the range of T it is none.
/// \throw boost::numeric::bad_numeric_cast from to at runtime, if Lua can not hold the value
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
struct value_cast<T> final
{
  [[nodiscard]] static constexpr auto to(T v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<T>;
};

///
/// A Lua number, an integer is one too.
///
template<std::floating_point T>
  requires detail::plain<T>
struct value_cast<T> final
{
  [[nodiscard]] static constexpr auto to(T v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<T>;
};

///
/// A Lua string.
///
template<>
struct value_cast<std::string> final
{
  [[nodiscard]] static constexpr auto to(const std::string& v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<std::string>;
};

///
/// A duration is its count.
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
struct value_cast<T> final
{
  [[nodiscard]] static constexpr auto to(const T& v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<T>;
};

///
/// A file system path is a string with '/' as separator.
///
template<>
struct value_cast<std::filesystem::path> final
{
  [[nodiscard]] static auto to(const std::filesystem::path& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::filesystem::path>;
};

///
/// nil or a T.
///
template<value_castable T>
struct value_cast<std::optional<T>> final
{
  [[nodiscard]] static constexpr auto to(const std::optional<T>& v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<std::optional<T>>;
};

///
/// A table of the values in sequence, one with other keys is none.
///
template<value_castable T>
struct value_cast<std::vector<T>> final
{
  [[nodiscard]] static constexpr auto to(const std::vector<T>& v) -> value;
  [[nodiscard]] static constexpr auto from(const value& v) -> std::optional<std::vector<T>>;
};

///
/// A table of the values by key. Lua takes any value but nil as key, so \p K is no std::optional.
///
template<value_castable K, value_castable T>
struct value_cast<std::map<K, T>> final
{
  [[nodiscard]] static auto to(const std::map<K, T>& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::map<K, T>>;
};

///
///
template<value_castable T>
constexpr auto to_value(const T& v) -> value
{
  return value_cast<T>::to(v);
}

///
///
template<value_castable T>
constexpr auto from_value(const value& v) -> std::optional<T>
{
  return value_cast<T>::from(v);
}

///
///
template<std::ranges::input_range R>
constexpr auto detail::all_or_none(R&& items)
  -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>
{
  const auto values = std::forward<R>(items) | std::ranges::to<std::vector>();
  if(!std::ranges::all_of(values, [](const auto& item) { return item.has_value(); }))
  {
    return std::nullopt;
  }
  return values | std::views::transform([](const auto& item) { return *item; }) |
         std::ranges::to<std::vector>();
}

///
///
constexpr auto value_cast<bool>::to(const bool v) -> value
{
  return value{v};
}

///
///
constexpr auto value_cast<bool>::from(const value& v) -> std::optional<bool>
{
  const auto held = std::get_if<bool>(&v);
  return (held != nullptr) ? std::optional{*held} : std::nullopt;
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
constexpr auto value_cast<T>::to(const T v) -> value
{
  return value{util::numeric_cast_rt<integer>(v)};
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
constexpr auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  const auto held = std::get_if<integer>(&v);
  return held && std::in_range<T>(*held) ? std::optional{static_cast<T>(*held)} : std::nullopt;
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
constexpr auto value_cast<T>::to(const T v) -> value
{
  return value{static_cast<number>(v)};
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
constexpr auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  if(const auto held = std::get_if<integer>(&v))
  {
    return static_cast<T>(*held);
  }
  const auto held = std::get_if<number>(&v);
  return (held != nullptr) ? std::optional{static_cast<T>(*held)} : std::nullopt;
}

///
///
constexpr auto value_cast<std::string>::to(const std::string& v) -> value
{
  return value{v};
}

///
///
constexpr auto value_cast<std::string>::from(const value& v) -> std::optional<std::string>
{
  const auto held = std::get_if<std::string>(&v);
  return (held != nullptr) ? std::optional{*held} : std::nullopt;
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
constexpr auto value_cast<T>::to(const T& v) -> value
{
  return to_value(v.count());
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
constexpr auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  return from_value<typename T::rep>(v).transform([](const auto count) { return T{count}; });
}

///
///
inline auto value_cast<std::filesystem::path>::to(const std::filesystem::path& v) -> value
{
  return to_value(v.generic_string());
}

///
///
inline auto value_cast<std::filesystem::path>::from(const value& v)
  -> std::optional<std::filesystem::path>
{
  return from_value<std::string>(v).transform([](const auto& s)
                                              { return std::filesystem::path{s}; });
}

///
///
template<value_castable T>
constexpr auto value_cast<std::optional<T>>::to(const std::optional<T>& v) -> value
{
  return v ? to_value(*v) : value{};
}

///
///
template<value_castable T>
constexpr auto value_cast<std::optional<T>>::from(const value& v) -> std::optional<std::optional<T>>
{
  if(std::holds_alternative<nil>(v))
  {
    return std::optional<T>{};
  }
  return from_value<T>(v).transform([](auto held) { return std::optional<T>{std::move(held)}; });
}

///
///
template<value_castable T>
constexpr auto value_cast<std::vector<T>>::to(const std::vector<T>& v) -> value
{
  auto result = table{};
  std::ranges::for_each(
    v,
    [&](const T& item)
    {
      result.entries.emplace_back(
        value{util::numeric_cast_rt<integer>(result.entries.size() + 1)}, to_value(item)
      );
    }
  );
  return value{std::move(result)};
}

///
///
template<value_castable T>
constexpr auto value_cast<std::vector<T>>::from(const value& v) -> std::optional<std::vector<T>>
{
  const auto held = std::get_if<table>(&v);
  // Else e.g. a table by key would be read as an empty list
  if((held == nullptr) || !held->is_sequence())
  {
    return std::nullopt;
  }
  return detail::all_or_none(
    held->sequence() | std::views::transform([](const value& item) { return from_value<T>(item); })
  );
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::to(const std::map<K, T>& v) -> value
{
  auto result = table{};
  std::ranges::for_each(
    v,
    [&](const auto& entry)
    { result.entries.emplace_back(to_value(entry.first), to_value(entry.second)); }
  );
  return value{std::move(result)};
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::from(const value& v) -> std::optional<std::map<K, T>>
{
  const auto held = std::get_if<table>(&v);
  if(held == nullptr)
  {
    return std::nullopt;
  }
  const auto entries = held->entries |
                       std::views::transform(
                         [](const table::entry& entry)
                         { return std::pair{from_value<K>(entry.key), from_value<T>(entry.item)}; }
                       ) |
                       std::ranges::to<std::vector>();
  if(!std::ranges::all_of(entries, [](const auto& entry) { return entry.first && entry.second; }))
  {
    return std::nullopt;
  }
  return entries |
         std::views::transform([](const auto& entry)
                               { return std::pair{*entry.first, *entry.second}; }) |
         std::ranges::to<std::map<K, T>>();
}

} // namespace arcstd::lua
