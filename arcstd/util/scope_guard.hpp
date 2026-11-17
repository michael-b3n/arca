#pragma once

#include "arcstd/export.hpp"

#include <functional>
#include <memory>

namespace arcstd::util
{

///
/// Calls its callable once, when destroyed or reset.
///
class ARCSTD_EXPORT scope_guard final
{
  // Variables
  std::move_only_function<void()> on_destruction_;

public: // Typedefs
  using on_destruction_type = decltype(on_destruction_);

public: // Structors
  scope_guard() = default;
  explicit scope_guard(on_destruction_type&& on_destruction);
  ~scope_guard() noexcept;
  scope_guard(scope_guard&& rhs) noexcept;
  scope_guard(const scope_guard&) = delete;

public: // Operators
  auto operator=(scope_guard&& rhs) & noexcept -> scope_guard&;
  auto operator=(const scope_guard&) -> scope_guard& = delete;

public: // Modifiers
  ///
  /// Call the callable now, if it was not called yet.
  ///
  auto reset() -> void;

private: // Implementation
  auto destruct() -> void;
};

///
/// Guard of a callable shared by several guards, it is called once the last of them is destroyed
/// or reset, \see creator.
///
class ARCSTD_EXPORT shared_scope_guard final
{
  // Variables
  bool is_initial_instance_{false};
  scope_guard instance_guard_;

public: // Typedefs
  ///
  /// Hands out the guards of one callable.
  ///
  class ARCSTD_EXPORT creator final
  {
    // Variables
    std::weak_ptr<scope_guard> instance_;

  public: // Structors
    creator() = default;

  public: // Operations
    ///
    /// \return a guard of \p on_destruction, or of the callable of the guards still alive, which
    /// keep theirs and drop \p on_destruction
    ///
    [[nodiscard]] auto create(scope_guard::on_destruction_type&& on_destruction)
      -> shared_scope_guard;
  };

public: // Structors
  shared_scope_guard() = default;
  ~shared_scope_guard() noexcept = default;
  shared_scope_guard(shared_scope_guard&&) noexcept = default;
  shared_scope_guard(const shared_scope_guard&) = delete;

public: // Operators
  auto operator=(shared_scope_guard&&) & noexcept -> shared_scope_guard& = default;
  auto operator=(const shared_scope_guard&) -> shared_scope_guard& = delete;

public: // Accessors
  ///
  /// \return true if the creator made the callable's guard for this guard
  ///
  [[nodiscard]] auto is_initial_instance() const -> bool;

public: // Modifiers
  ///
  /// Let go of the callable, it is called if this was the last guard.
  ///
  auto reset() -> void;

private: // Structors
  shared_scope_guard(scope_guard instance_guard, bool is_initial_instance);
};

} // namespace arcstd::util
