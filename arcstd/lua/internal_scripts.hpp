#pragma once

#if !defined(ARCSTD_INTERNAL)
  #error "arcstd/lua/internal_scripts.hpp is a private header of arcstd"
#endif

#include "arcstd/lua/embedded_script.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace arcstd::lua::internal
{

///
/// \return the scripts of arcstd/lua/internal, they set up the state
///
[[nodiscard]] auto all() -> std::span<const embedded_script>;

///
/// \return the script with the file name \p name
///
[[nodiscard]] auto find(std::string_view name) -> std::optional<embedded_script>;

} // namespace arcstd::lua::internal
