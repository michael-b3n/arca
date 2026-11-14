#include "arcstd/lua/object_cast.hpp"
#include "arcstd/util/numeric_cast.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::lua
{
namespace
{

// A value holds the integers and numbers of Lua as they are
static_assert(std::is_same_v<lua_Integer, value::integer_type>);
static_assert(std::is_same_v<lua_Number, value::number_type>);

// No C++ type a script value converts to is nested this deep, and a table containing itself ends here
constexpr auto max_table_depth = std::size_t{100};

///
/// \return a copy of \p object, tables below \p depth more levels are opaque
///
[[nodiscard]] auto to_value(const sol::object& object, const std::size_t depth) -> value
{
  switch(object.get_type())
  {
  case sol::type::lua_nil:
  case sol::type::none: return value{};
  case sol::type::boolean: return value{object.as<bool>()};
  case sol::type::number:
    // With the safeties on, sol takes integral numbers only
    if(const auto integer = object.as<sol::optional<lua_Integer>>())
    {
      return value{*integer};
    }
    return value{object.as<lua_Number>()};
  case sol::type::string: return value{object.as<std::string>()};
  case sol::type::table:
  {
    if(depth == 0)
    {
      return value{value_opaque{}};
    }
    const auto table = object.as<sol::table>();
    auto result = value_table{};
    table.for_each([&](const sol::object& key, const sol::object& item)
                   { result.entries.emplace_back(to_value(key, depth - 1), to_value(item, depth - 1)); });
    return value{std::move(result)};
  }
  case sol::type::function:
  case sol::type::userdata:
  case sol::type::lightuserdata:
  case sol::type::thread:
  case sol::type::poly: return value{value_opaque{}};
  }
  return value{value_opaque{}};
}

} // namespace

///
///
auto to_object(sol::state_view lua, const value& v) -> sol::object
{
  return std::visit(
    [&]<typename T>(const T& held) -> sol::object
    {
      if constexpr(std::is_same_v<T, value_nil> || std::is_same_v<T, value_opaque>)
      {
        return sol::make_object(lua, sol::lua_nil);
      }
      else if constexpr(std::is_same_v<T, value_table>)
      {
        const auto sequence = held.length();
        auto table = lua.create_table(numeric_cast<int>(sequence), numeric_cast<int>(held.entries.size() - sequence));
        std::ranges::for_each(
          held.entries, [&](const auto& entry) { table[to_object(lua, entry.first)] = to_object(lua, entry.second); }
        );
        return table;
      }
      else
      {
        return sol::make_object(lua, held);
      }
    },
    v
  );
}

///
///
auto to_value(const sol::object& object) -> value
{
  return to_value(object, max_table_depth);
}

///
///
auto to_object_function(function function) -> object_function
{
  return [function = std::move(function)](sol::this_state lua, sol::variadic_args arguments)
  {
    const auto state = sol::state_view{static_cast<lua_State*>(lua)};
    const auto values =
      std::views::iota(std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(arguments.size())) |
      std::views::transform([&](const std::ptrdiff_t index) { return to_value(arguments[index].get<sol::object>()); }) |
      std::ranges::to<std::vector>();
    auto results = sol::variadic_results{};
    std::ranges::for_each(function(values), [&](const value& result) { results.push_back(to_object(state, result)); });
    return results;
  };
}

} // namespace arcstd::lua
