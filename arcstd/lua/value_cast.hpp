#pragma once

#include "arcstd/lua/value.hpp"
#include "arcstd/meta/chrono.hpp"
#include "arcstd/util/numeric_cast.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
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
/// Conversion between a C++ value and the Lua value a script sees: to(v) -> value and
/// from(value) -> std::optional<T>, std::nullopt if the value is no T. Both copy.
/// Only plain value types convert, no references, pointers, views or const types.
///
template<typename T>
struct value_cast final
{
  // Constants
  static constexpr auto valid = false;
};

///
/// Types value_cast converts.
///
template<typename T>
concept value_castable = value_cast<T>::valid;

namespace detail
{

///
/// A value type of its own, no reference and neither const nor volatile.
///
template<typename T>
concept plain = std::same_as<T, std::remove_cvref_t<T>>;

///
/// \return the values of \p items, std::nullopt if one of them is none
///
template<std::ranges::input_range R>
[[nodiscard]] auto all_or_none(R&& items) -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>;

///
/// \return the values of the sequence of \p table, from index 1 to its length, nil where it has none
///
[[nodiscard]] auto sequence(const value_table& table) -> std::vector<value>;

} // namespace detail

///
/// A Lua boolean.
///
template<>
struct value_cast<bool> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(bool v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<bool>;
};

///
/// A Lua integer, a float is none even of an integral value. Out of the range of T it is none.
/// \throw boost::numeric::bad_numeric_cast from to, if Lua can not hold the value
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(T v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<T>;
};

///
/// A Lua number.
///
template<std::floating_point T>
  requires detail::plain<T>
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(T v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<T>;
};

///
/// A Lua string.
///
template<>
struct value_cast<std::string> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const std::string& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::string>;
};

///
/// A duration is its count.
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
struct value_cast<T> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const T& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<T>;
};

///
/// A file system path is a string with '/' as separator.
///
template<>
struct value_cast<std::filesystem::path> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const std::filesystem::path& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::filesystem::path>;
};

///
/// nil or a T.
///
template<value_castable T>
struct value_cast<std::optional<T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const std::optional<T>& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::optional<T>>;
};

///
/// A table of the values in sequence, one with other keys is none.
///
template<value_castable T>
struct value_cast<std::vector<T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const std::vector<T>& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::vector<T>>;
};

///
/// A table of the values by key. Lua takes any value but nil as key, so \p K is no std::optional.
///
template<value_castable K, value_castable T>
struct value_cast<std::map<K, T>> final
{
  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const std::map<K, T>& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<std::map<K, T>>;
};

///
///
inline auto detail::sequence(const value_table& table) -> std::vector<value>
{
  auto result = std::vector<value>(table.length());
  std::ranges::for_each(
    table.entries,
    [&](const auto& entry)
    {
      const auto index = std::get_if<value::integer_type>(&entry.first);
      if(index && *index >= 1 && std::cmp_less_equal(*index, result.size()))
      {
        result[static_cast<std::size_t>(*index - 1)] = entry.second;
      }
    }
  );
  return result;
}

///
///
template<std::ranges::input_range R>
auto detail::all_or_none(R&& items) -> std::optional<std::vector<typename std::ranges::range_value_t<R>::value_type>>
{
  const auto values = std::forward<R>(items) | std::ranges::to<std::vector>();
  if(!std::ranges::all_of(values, [](const auto& value) { return value.has_value(); }))
  {
    return std::nullopt;
  }
  return values | std::views::transform([](const auto& value) { return *value; }) | std::ranges::to<std::vector>();
}

///
///
inline auto value_cast<bool>::to(const bool v) -> value
{
  return value{v};
}

///
///
inline auto value_cast<bool>::from(const value& v) -> std::optional<bool>
{
  const auto boolean = std::get_if<bool>(&v);
  return boolean ? std::optional{*boolean} : std::nullopt;
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
auto value_cast<T>::to(const T v) -> value
{
  return value{numeric_cast<value::integer_type>(v)};
}

///
///
template<std::integral T>
  requires(detail::plain<T> && !std::same_as<T, bool>)
auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  const auto integer = std::get_if<value::integer_type>(&v);
  return integer && std::in_range<T>(*integer) ? std::optional{static_cast<T>(*integer)} : std::nullopt;
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
auto value_cast<T>::to(const T v) -> value
{
  return value{static_cast<value::number_type>(v)};
}

///
///
template<std::floating_point T>
  requires detail::plain<T>
auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  // An integer is a number too
  if(const auto integer = std::get_if<value::integer_type>(&v))
  {
    return static_cast<T>(*integer);
  }
  const auto number = std::get_if<value::number_type>(&v);
  return number ? std::optional{static_cast<T>(*number)} : std::nullopt;
}

///
///
inline auto value_cast<std::string>::to(const std::string& v) -> value
{
  return value{v};
}

///
///
inline auto value_cast<std::string>::from(const value& v) -> std::optional<std::string>
{
  const auto string = std::get_if<std::string>(&v);
  return string ? std::optional{*string} : std::nullopt;
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
auto value_cast<T>::to(const T& v) -> value
{
  return value_cast<typename T::rep>::to(v.count());
}

///
///
template<typename T>
  requires(detail::plain<T> && meta::is_duration_v<T>)
auto value_cast<T>::from(const value& v) -> std::optional<T>
{
  return value_cast<typename T::rep>::from(v).transform([](const auto count) { return T{count}; });
}

///
///
inline auto value_cast<std::filesystem::path>::to(const std::filesystem::path& v) -> value
{
  return value_cast<std::string>::to(v.generic_string());
}

///
///
inline auto value_cast<std::filesystem::path>::from(const value& v) -> std::optional<std::filesystem::path>
{
  return value_cast<std::string>::from(v).transform([](const auto& s) { return std::filesystem::path{s}; });
}

///
///
template<value_castable T>
auto value_cast<std::optional<T>>::to(const std::optional<T>& v) -> value
{
  return v ? value_cast<T>::to(*v) : value{};
}

///
///
template<value_castable T>
auto value_cast<std::optional<T>>::from(const value& v) -> std::optional<std::optional<T>>
{
  if(std::holds_alternative<value_nil>(v))
  {
    return std::optional<T>{};
  }
  return value_cast<T>::from(v).transform([](auto value) { return std::optional<T>{std::move(value)}; });
}

///
///
template<value_castable T>
auto value_cast<std::vector<T>>::to(const std::vector<T>& v) -> value
{
  auto table = value_table{};
  std::ranges::for_each(
    v,
    [&](const T& item)
    { table.entries.emplace_back(value{numeric_cast<value::integer_type>(table.entries.size() + 1)}, value_cast<T>::to(item)); }
  );
  return value{std::move(table)};
}

///
///
template<value_castable T>
auto value_cast<std::vector<T>>::from(const value& v) -> std::optional<std::vector<T>>
{
  const auto table = std::get_if<value_table>(&v);
  // Else e.g. a table by key would be read as an empty list
  if(!table || table->entries.size() != table->length())
  {
    return std::nullopt;
  }
  return detail::all_or_none(
    detail::sequence(*table) | std::views::transform([](const value& item) { return value_cast<T>::from(item); })
  );
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::to(const std::map<K, T>& v) -> value
{
  auto table = value_table{};
  std::ranges::for_each(
    v, [&](const auto& entry) { table.entries.emplace_back(value_cast<K>::to(entry.first), value_cast<T>::to(entry.second)); }
  );
  return value{std::move(table)};
}

///
///
template<value_castable K, value_castable T>
auto value_cast<std::map<K, T>>::from(const value& v) -> std::optional<std::map<K, T>>
{
  const auto table = std::get_if<value_table>(&v);
  if(!table)
  {
    return std::nullopt;
  }
  const auto entries =
    table->entries |
    std::views::transform([](const auto& entry)
                          { return std::pair{value_cast<K>::from(entry.first), value_cast<T>::from(entry.second)}; }) |
    std::ranges::to<std::vector>();
  if(!std::ranges::all_of(entries, [](const auto& entry) { return entry.first && entry.second; }))
  {
    return std::nullopt;
  }
  return entries | std::views::transform([](const auto& entry) { return std::pair{*entry.first, *entry.second}; }) |
         std::ranges::to<std::map<K, T>>();
}

} // namespace arcstd::lua
