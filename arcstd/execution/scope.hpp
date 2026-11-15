#pragma once

#include "arcstd/export.hpp"

#include <functional>
#include <memory>
#include <mutex>

namespace arcstd::execution
{

///
/// Scope holding launched work, \see launch. Destruction requests a stop and
/// waits for the work, so the work may refer to whatever outlives the scope.
///
class ARCSTD_EXPORT scope final
{
  // Friends
  friend ARCSTD_EXPORT auto launch(std::move_only_function<void()> work, scope& scope) -> void;

  // Typedefs
  struct impl;

  // Variables
  mutable std::mutex mtx_;
  bool stopped_{false};
  const std::unique_ptr<impl> impl_;

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
