#pragma once

#include "arcstd/export.hpp"
#include "arcstd/signal/common.hpp"

#include <mutex>
#include <vector>

namespace arcstd::signal
{

///
/// Keeps connections and disconnects them when destroyed.
///
class ARCSTD_CORE_EXPORT connection_store final
{
  // Variables
  mutable std::mutex mtx_;
  std::vector<scoped_connection_type> connections_;

public: // Structors
  connection_store() = default;
  ~connection_store() noexcept;

public: // Modifiers
  ///
  /// Keep \p con until clear or destruction.
  ///
  auto store(scoped_connection_type con) -> void;

  ///
  /// Disconnect all connections.
  ///
  auto clear() -> void;
};

} // namespace arcstd::signal
