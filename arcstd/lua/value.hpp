#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::lua
{

struct value;

///
/// Lua nil.
///
struct value_nil final
{
  // Operators
  auto operator==(const value_nil&) const -> bool = default;
};

///
/// A Lua value C++ takes no copy of: a function, a userdata or a thread, or a table nested too deep.
///
struct value_opaque final
{
  // Operators
  auto operator==(const value_opaque&) const -> bool = default;
};

///
/// Lua table, its entries by key.
///
struct value_table final
{
  // Variables
  std::vector<std::pair<value, value>> entries;

  // Accessors
  ///
  /// \return the length of the sequence of the table: how many of the keys 1, 2, 3, ... it has in a row
  ///
  [[nodiscard]] auto length() const -> std::size_t;
};

///
/// Copy of a Lua value, what a script hands to C++ and gets from it, \see value_cast. Nil by default.
/// It holds no reference into the Lua state and may outlive it.
///
struct value final : std::variant<value_nil, bool, std::int64_t, double, std::string, value_table, value_opaque>
{
  // Typedefs
  using integer_type = std::int64_t;
  using number_type = double;

  // Structors
  using variant::variant;
};

///
///
inline auto value_table::length() const -> std::size_t
{
  const auto is_index = [](const value& key)
  {
    const auto index = std::get_if<value::integer_type>(&key);
    return index && *index > 0;
  };
  auto indices = entries | std::views::keys | std::views::filter(is_index) |
                 std::views::transform([](const value& key) { return std::get<value::integer_type>(key); }) |
                 std::ranges::to<std::vector>();
  std::ranges::sort(indices);
  const auto end = std::ranges::mismatch(indices, std::views::iota(value::integer_type{1})).in1;
  return static_cast<std::size_t>(std::ranges::distance(indices.begin(), end));
}

} // namespace arcstd::lua
