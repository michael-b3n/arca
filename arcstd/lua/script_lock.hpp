#pragma once

#include "arcstd/export.hpp"

#include <memory>

namespace arcstd::lua
{

///
/// Lock of the scripts, \see lua_engine::lock. As long as it lives no script runs on another thread
/// and nothing is registered or taken back there. The thread holding it may lock again.
///
class ARCSTD_LUA_EXPORT script_lock final
{
  // Variables
  std::shared_ptr<const void> state_;

public: // Structors
  ///
  /// Lock for as long as \p state lives, the locked Lua state.
  ///
  explicit script_lock(std::shared_ptr<const void> state);
  script_lock(const script_lock&) = delete;
  script_lock(script_lock&&) noexcept = default;
  auto operator=(const script_lock&) -> script_lock& = delete;
  auto operator=(script_lock&&) noexcept -> script_lock& = delete;
  ~script_lock() noexcept = default;
};

} // namespace arcstd::lua
