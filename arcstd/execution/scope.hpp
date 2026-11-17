#pragma once

#include "arcstd/export.hpp"

#include <functional>
#include <memory>
#include <mutex>

namespace arcstd::execution
{

///
/// Scope of launched work, \see launch. Destroyed, it requests a stop and waits for the work, so
/// the work may refer to whatever outlives the scope.
///
class ARCSTD_CORE_EXPORT scope final
{
  // Friends
  friend ARCSTD_CORE_EXPORT auto launch(std::move_only_function<void()> work, scope& scope) -> void;

  // Typedefs
  struct impl;

  // Variables
  const std::unique_ptr<impl> impl_;
  mutable std::mutex mtx_;
  bool stopped_{false};

public: // Structors
  scope();
  ~scope() noexcept;

public: // Modifiers
  ///
  /// Request a stop and wait for the launched work. Work launched afterwards is dropped.
  /// \warning Must not be called by the work itself.
  ///
  auto stop() noexcept -> void;
};

} // namespace arcstd::execution
