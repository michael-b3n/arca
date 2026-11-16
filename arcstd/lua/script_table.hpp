#pragma once

#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_cast.hpp"
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
template<util::string_literal Key, value_castable T>
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
} && value_castable<typename F::type>;

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
struct value_cast<script_table<Fields...>> final
{
  // Typedefs
  using table_type = script_table<Fields...>;

  // Constants
  static constexpr auto valid = true;

  // Functions
  [[nodiscard]] static auto to(const table_type& v) -> value;
  [[nodiscard]] static auto from(const value& v) -> std::optional<table_type>;

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
auto value_cast<script_table<Fields...>>::to(const table_type& v) -> value
{
  auto table = value_table{};
  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    (table.entries.emplace_back(
       value{std::string{Fields::key}}, value_cast<typename Fields::type>::to(std::get<I>(v.values()))
     ),
     ...);
  }(std::index_sequence_for<Fields...>{});
  return value{std::move(table)};
}

///
///
template<script_field... Fields>
auto value_cast<script_table<Fields...>>::from(const value& v) -> std::optional<table_type>
{
  const auto nil = std::holds_alternative<value_nil>(v);
  if(!nil && !std::holds_alternative<value_table>(v))
  {
    return std::nullopt;
  }
  const auto table = nil ? value_table{} : std::get<value_table>(v);
  const auto known = std::ranges::all_of(
    table.entries,
    [](const auto& entry)
    {
      const auto name = value_cast<std::string>::from(entry.first);
      return name && std::ranges::contains(table_type::keys, std::string_view{*name});
    }
  );
  // The value of the field \p key, nil if the table has none
  const auto field = [&](const std::string_view key)
  {
    const auto it =
      std::ranges::find_if(table.entries, [&](const auto& entry) { return value_cast<std::string>::from(entry.first) == key; });
    return it != std::ranges::end(table.entries) ? it->second : value{};
  };
  auto result = table_type{};
  const auto read = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return (assign(std::get<I>(result.values()), field(Fields::key)) && ...); };
  return known && read(std::index_sequence_for<Fields...>{}) ? std::optional{std::move(result)} : std::nullopt;
}

///
///
template<script_field... Fields>
template<typename T>
auto value_cast<script_table<Fields...>>::assign(T& target, const value& v) -> bool
{
  auto value = value_cast<T>::from(v);
  if(value)
  {
    target = std::move(*value);
  }
  return value.has_value();
}

} // namespace arcstd::lua
