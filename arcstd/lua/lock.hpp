#pragma once

#include <memory>

namespace arcstd::lua
{

///
/// Lock of the scripts, \see workflow::workflow_script::lock. As long as it lives no script runs on another thread
/// and nothing is registered or taken back there. The thread holding it may lock again.
///
class lock final
{
  // Variables
  std::shared_ptr<const void> state_;

public: // Structors
  ///
  /// Lock for as long as \p state lives, the locked Lua state.
  ///
  explicit lock(std::shared_ptr<const void> state);
  lock(const lock&) = delete;
  lock(lock&&) noexcept = default;
  auto operator=(const lock&) -> lock& = delete;
  auto operator=(lock&&) noexcept -> lock& = delete;
  ~lock() noexcept = default;
};

} // namespace arcstd::lua
