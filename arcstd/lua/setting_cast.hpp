#pragma once

#include "arcstd/framework/setting.hpp"
#include "arcstd/framework/setting_common.hpp"
#include "arcstd/lua/function_cast.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

///
/// Names of the functions a setting offers to scripts.
///
namespace arcstd::lua::names
{
inline constexpr std::string_view function_get = "get";
inline constexpr std::string_view function_set = "set";
inline constexpr std::string_view function_postfix = "postfix";
} // namespace arcstd::lua::names

namespace arcstd::lua
{

///
/// Functions of a table for scripts, by name.
///
using function_table = std::vector<std::pair<std::string, function>>;

///
/// The functions \p setting offers to scripts: get(), set(value) and postfix(), for as long as \p registered lives,
/// \see function_cast. \p setting must live as long.
/// \return the functions by name
///
template<framework::underlying_setting_type T>
[[nodiscard]] auto setting_cast(framework::setting<T>& setting, const std::weak_ptr<const void>& registered) -> function_table
{
  using erased_type = framework::setting_type_erased_type_from<T>;

  auto functions = function_table{};
  ///
  /// \lua <setting>.get()
  /// \return the value of the setting
  ///
  functions.emplace_back(
    names::function_get,
    function_cast(
      [&setting]() -> erased_type { return framework::create_setting_value_converter<T, erased_type>()(setting.value()); },
      registered
    )
  );
  ///
  /// \lua <setting>.set(value)
  /// Changes the setting, like the user does in the settings.
  /// \param value the new value
  /// \return false if the setting rejected \p value or limited it to its range
  ///
  functions.emplace_back(
    names::function_set,
    function_cast(
      [&setting](const erased_type& value) -> bool
      { return setting.value(framework::create_setting_value_converter<erased_type, T>()(value)); },
      registered
    )
  );
  ///
  /// \lua <setting>.postfix()
  /// \return the text shown after the value, e.g. its unit, or an empty text
  ///
  functions.emplace_back(names::function_postfix, function_cast([&setting]() { return setting.postfix; }, registered));
  return functions;
}

} // namespace arcstd::lua
