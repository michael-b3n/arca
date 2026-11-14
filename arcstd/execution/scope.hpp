#pragma once

#include "arcstd/export.hpp"

#include <functional>
#include <memory>
#include <mutex>

namespace arcstd::execution
{

class scope;

namespace detail
{

struct scope_data;

///
/// Run \p work on the thread pool, detached in \p scope, \see execution::launch
///
ARCSTD_EXPORT auto launch(std::move_only_function<void()> work, scope& scope) -> void;

} // namespace detail

///
/// Scope holding launched work, \see launch. Destruction requests a stop and
/// waits for the work, so the work may refer to whatever outlives the scope.
///
class ARCSTD_EXPORT scope final
{
  // Friends
  friend ARCSTD_EXPORT auto detail::launch(std::move_only_function<void()> work, scope& scope) -> void;

  // Variables
  mutable std::mutex mtx_;
  bool stopped_{false};
  const std::unique_ptr<detail::scope_data> data_;

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
