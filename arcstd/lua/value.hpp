#pragma once

#include <cstddef>
#include <cstdint>
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
/// Lua table: its entries by key, and its length as Lua counts it, the border of its sequence.
/// Handed to Lua only the entries count.
///
struct value_table final
{
  // Variables
  std::vector<std::pair<value, value>> entries;
  std::size_t length{0};

  // Operators
  auto operator==(const value_table& other) const -> bool;
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
inline auto value_table::operator==(const value_table& other) const -> bool
{
  return length == other.length && entries == other.entries;
}

} // namespace arcstd::lua
