#pragma once

#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_conversion.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/string_literal.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace arcstd::lua
{

///
/// Field \p Key of a script_table, a value of type \p T.
///
template<util::string_literal Key, value_convertible T>
struct field final
{
  // Typedefs
  using type = T;

  // Constants
  static constexpr auto key = Key.view();
};

///
/// Field of a script_table: its key and the type of its value, \see field.
///
template<typename F>
concept script_field = requires {
  { F::key } -> std::convertible_to<std::string_view>;
  typename F::type;
} && value_convertible<typename F::type>;

///
/// Table of the values of \p Fields, a Lua table with a key per field for scripts.
///
template<script_field... Fields>
class script_table final
{
  // Typedefs
  using values_type = std::tuple<typename Fields::type...>;

  // Variables
  values_type values_;

public: // Structors
  script_table() = default;
  // NOLINTNEXTLINE(misc-explicit-constructor) a table is written as a braced list of its fields
  script_table(typename Fields::type... values)
    requires(sizeof...(Fields) > 0);

public: // Operators
  auto operator==(const script_table& other) const -> bool = default;

public: // Constants
  static constexpr auto keys = std::array<std::string_view, sizeof...(Fields)>{Fields::key...};

public: // Accessors
  ///
  /// \return the value of the field \p Key
  ///
  template<util::string_literal Key>
  [[nodiscard]] auto get() const -> const auto&;

  ///
  /// \return the value of the field \p Key
  ///
  template<util::string_literal Key>
  [[nodiscard]] auto get() -> auto&;

  ///
  /// \return const references of the values of all fields
  ///
  [[nodiscard]] auto values() const -> const values_type&;

  ///
  /// \return references to the values of all fields
  ///
  [[nodiscard]] auto values() -> values_type&;

private: // Implementation
  template<util::string_literal Key>
  [[nodiscard]] static consteval auto index() -> std::size_t;
};

///
/// A table with the keys of the fields. Reading, nil is an empty table and a key of no field makes it none.
///
template<script_field... Fields>
struct value_conversion<script_table<Fields...>> final
{
  // Typedefs
  using script_table_type = script_table<Fields...>;

  // Functions
  [[nodiscard]] static auto to_value(const script_table_type& v) -> value;
  [[nodiscard]] static auto from_value(const value& v) -> std::optional<script_table_type>;

private: // Implementation
  template<typename T>
  [[nodiscard]] static auto assign(T& target, const value& v) -> bool;
};

///
///
template<script_field... Fields>
script_table<Fields...>::script_table(typename Fields::type... values)
  requires(sizeof...(Fields) > 0)
  : values_{std::move(values)...}
{
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
auto script_table<Fields...>::get() const -> const auto&
{
  return std::get<index<Key>()>(values_);
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
auto script_table<Fields...>::get() -> auto&
{
  return std::get<index<Key>()>(values_);
}

///
///
template<script_field... Fields>
auto script_table<Fields...>::values() const -> const values_type&
{
  return values_;
}

///
///
template<script_field... Fields>
auto script_table<Fields...>::values() -> values_type&
{
  return values_;
}

///
///
template<script_field... Fields>
template<util::string_literal Key>
consteval auto script_table<Fields...>::index() -> std::size_t
{
  const auto it = std::ranges::find(keys, Key.view());
  if(it == std::ranges::end(keys))
  {
    throw util::exception{"key not found in script_table"};
  }
  return static_cast<std::size_t>(std::ranges::distance(std::ranges::begin(keys), it));
}

///
///
template<script_field... Fields>
auto value_conversion<script_table<Fields...>>::to_value(const script_table_type& v) -> value
{
  auto result = table{};
  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    (result.entries.emplace_back(value{std::string{Fields::key}}, lua::to_value(std::get<I>(v.values()))), ...);
  }(std::index_sequence_for<Fields...>{});
  return value{std::move(result)};
}

///
///
template<script_field... Fields>
auto value_conversion<script_table<Fields...>>::from_value(const value& v) -> std::optional<script_table_type>
{
  // nil is an empty table, so a script may leave out a table of optional fields only
  const auto empty = table{};
  const auto* const held = std::holds_alternative<nil>(v) ? &empty : std::get_if<table>(&v);
  if(held == nullptr)
  {
    return std::nullopt;
  }
  const auto is_field = [](const table::entry& entry)
  {
    const auto key = std::get_if<std::string>(&entry.key);
    return key && std::ranges::contains(script_table_type::keys, std::string_view{*key});
  };
  if(!std::ranges::all_of(held->entries, is_field))
  {
    return std::nullopt;
  }
  auto result = script_table_type{};
  const auto assigned = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return (assign(std::get<I>(result.values()), held->field(Fields::key)) && ...); }(std::index_sequence_for<Fields...>{});
  return assigned ? std::optional{std::move(result)} : std::nullopt;
}

///
///
template<script_field... Fields>
template<typename T>
auto value_conversion<script_table<Fields...>>::assign(T& target, const value& v) -> bool
{
  auto converted = lua::from_value<T>(v);
  if(converted)
  {
    target = std::move(*converted);
  }
  return converted.has_value();
}

} // namespace arcstd::lua
