#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::lua
{

struct value;

///
/// Lua nil: no value, e.g. an argument left out or
/// the value of a key a table does not have.
///
struct nil final
{};

///
/// A Lua value C++ gets no copy of: a function, a userdata, a thread, or a
/// table nested too deep or containing itself. It is no nil, so it is not
/// taken for a value left out. Handed back to Lua, it is nil.
///
struct unsupported final
{};

///
/// A Lua integer.
///
using integer = std::int64_t;

///
/// A Lua number that is no integer.
///
using number = double;

///
/// Copy of a Lua table, its entries in no order.
///
struct table final
{
  // Typedefs
  struct entry;

  // Variables
  std::vector<entry> entries;

  // Accessors
  ///
  /// \return how many of the keys 1, 2, 3, ... the table has in a row, the length # gives in Lua
  ///
  [[nodiscard]] auto length() const -> std::size_t;

  ///
  /// \return true if the table is a list: its keys are 1 to its length and no others
  ///
  [[nodiscard]] auto is_sequence() const -> bool;

  ///
  /// \return the values of the keys 1 to the length, in that order
  ///
  [[nodiscard]] auto sequence() const -> std::vector<value>;

  ///
  /// \return the value of the key \p name, nil if the table has none
  ///
  [[nodiscard]] auto field(std::string_view name) const -> value;
};

///
/// Copy of a Lua value, what a script hands to C++ and gets from it,
/// \see value_cast. Nil by default. It holds no reference into
/// the Lua state, so it may outlive it.
///
struct value final : std::variant<nil, bool, integer, number, std::string, table, unsupported>
{
  // Structors
  using variant::variant;
};

///
/// Entry of a table, its key and the value it holds.
///
struct table::entry final
{
  value key;
  value item;
};

///
///
inline auto table::length() const -> std::size_t
{
  const auto index = [](const entry& e) { return std::get_if<integer>(&e.key); };
  auto indices = entries | std::views::transform(index) |
                 std::views::filter([](const integer* const i) { return i != nullptr && *i > 0; }) |
                 std::views::transform([](const integer* const i) { return *i; }) |
                 std::ranges::to<std::vector>();
  std::ranges::sort(indices);
  const auto end = std::ranges::mismatch(indices, std::views::iota(integer{1})).in1;
  return static_cast<std::size_t>(std::ranges::distance(indices.begin(), end));
}

///
///
inline auto table::is_sequence() const -> bool
{
  return entries.size() == length();
}

///
///
inline auto table::sequence() const -> std::vector<value>
{
  auto result = std::vector<value>(length());
  std::ranges::for_each(
    entries,
    [&](const entry& e)
    {
      const auto index = std::get_if<integer>(&e.key);
      if(index && *index >= 1 && std::cmp_less_equal(*index, result.size()))
      {
        result[static_cast<std::size_t>(*index - 1)] = e.item;
      }
    }
  );
  return result;
}

///
///
inline auto table::field(const std::string_view name) const -> value
{
  const auto it = std::ranges::find_if(
    entries,
    [&](const entry& e)
    {
      const auto key = std::get_if<std::string>(&e.key);
      return key && *key == name;
    }
  );
  return it != entries.end() ? it->item : value{};
}

} // namespace arcstd::lua
