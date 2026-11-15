#pragma once

#include "arcstd/lua/function_cast.hpp"
#include "arcstd/setting/setting.hpp"
#include "arcstd/setting/setting_common.hpp"

namespace arcstd::lua
{

///
/// The functions \p setting offers to scripts: get(), set(value) and postfix(), \see function_cast. \p setting must
/// live as long as they are registered.
/// \return the functions by name
///
template<setting::underlying_setting_type T>
[[nodiscard]] auto setting_functions(setting::setting<T>& setting) -> function_table
{
  using erased_type = setting::setting_type_erased_type_from<T>;

  auto functions = function_table{};
  ///
  /// \lua <setting>.get()
  /// \return the value of the setting
  ///
  functions.emplace(
    "get",
    function_cast(
      [&setting] -> erased_type { return setting::create_setting_value_converter<T, erased_type>()(setting.value()); }
    )
  );
  ///
  /// \lua <setting>.set(value)
  /// Changes the setting, like the user does in the settings.
  /// \param value the new value
  /// \return false if the setting rejected \p value or limited it to its range
  ///
  functions.emplace(
    "set",
    function_cast(
      [&setting](const erased_type& value) -> bool
      { return setting.value(setting::create_setting_value_converter<erased_type, T>()(value)); }
    )
  );
  ///
  /// \lua <setting>.postfix()
  /// \return the text shown after the value, e.g. its unit, or an empty text
  ///
  functions.emplace("postfix", function_cast([&setting] { return setting.postfix; }));
  return functions;
}

} // namespace arcstd::lua
