#include "arcstd/lua/sol_conversion.hpp"
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
static_assert(std::is_same_v<lua_Integer, integer>);
static_assert(std::is_same_v<lua_Number, number>);

// No C++ type a script value converts to is nested this deep,
// and a table containing itself ends here
constexpr auto max_table_depth = std::size_t{100};

///
/// \return a copy of \p object, tables below \p depth more levels are unsupported
///
[[nodiscard]] auto from_sol(const sol::object& object, const std::size_t depth) -> value
{
  switch(object.get_type())
  {
  case sol::type::lua_nil: [[fallthrough]];
  case sol::type::none: return value{};
  case sol::type::boolean: return value{object.as<bool>()};
  case sol::type::number:
    // With the safeties on, sol takes integral numbers only
    if(const auto held = object.as<sol::optional<lua_Integer>>())
    {
      return value{*held};
    }
    return value{object.as<lua_Number>()};
  case sol::type::string: return value{object.as<std::string>()};
  case sol::type::table:
  {
    if(depth == 0)
    {
      return value{unsupported{}};
    }
    auto result = table{};
    object.as<sol::table>().for_each(
      [&](const sol::object& key, const sol::object& item)
      { result.entries.emplace_back(from_sol(key, depth - 1), from_sol(item, depth - 1)); }
    );
    return value{std::move(result)};
  }
  case sol::type::function: [[fallthrough]];
  case sol::type::userdata: [[fallthrough]];
  case sol::type::lightuserdata: [[fallthrough]];
  case sol::type::thread: [[fallthrough]];
  case sol::type::poly: return value{unsupported{}};
  }
  return value{unsupported{}};
}

} // namespace

///
///
auto to_sol(sol::state_view lua, const value& v) -> sol::object
{
  return std::visit(
    [&]<typename T>(const T& held) -> sol::object
    {
      if constexpr(std::is_same_v<T, nil> || std::is_same_v<T, unsupported>)
      {
        return sol::make_object(lua, sol::lua_nil);
      }
      else if constexpr(std::is_same_v<T, table>)
      {
        const auto sequence = held.length();
        auto result = lua.create_table(
          util::numeric_cast_rt<int>(sequence),
          util::numeric_cast_rt<int>(held.entries.size() - sequence)
        );
        std::ranges::for_each(
          held.entries,
          [&](const table::entry& entry)
          { result[to_sol(lua, entry.key)] = to_sol(lua, entry.item); }
        );
        return result;
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
auto from_sol(const sol::object& object) -> value
{
  return from_sol(object, max_table_depth);
}

///
///
auto to_sol(function function) -> sol_function
{
  return [function = std::move(function)](sol::this_state lua, sol::variadic_args arguments)
  {
    const auto state = sol::state_view{static_cast<lua_State*>(lua)};
    const auto values =
      std::views::iota(std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(arguments.size())) |
      std::views::transform([&](const std::ptrdiff_t index)
                            { return from_sol(arguments[index].get<sol::object>()); }) |
      std::ranges::to<std::vector>();
    auto results = sol::variadic_results{};
    std::ranges::for_each(
      function(values), [&](const value& result) { results.push_back(to_sol(state, result)); }
    );
    return results;
  };
}

} // namespace arcstd::lua
