#pragma once

#include <string_view>

namespace arcstd::lua
{

///
/// Lua script compiled into the binary.
///
struct embedded_script final
{
  std::string_view name;
  std::string_view code;
};

} // namespace arcstd::lua
