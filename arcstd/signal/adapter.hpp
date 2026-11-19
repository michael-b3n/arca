#pragma once

#include "arcstd/signal/common.hpp"

#include <concepts>
#include <functional>

namespace arcstd::signal
{
namespace detail
{
// clang-format off
struct any_return_type{};
struct any_argument_type{};
using any_signal_type = signal_type<any_return_type(any_argument_type)>;
// clang-format on
} // namespace detail

///
/// Concept for what queues the slots of the adapter, \see queued_connections.
///
template<typename T>
concept connector_kind = requires(
  T t,
  detail::any_signal_type& sig,
  detail::any_signal_type::slot_type slot,
  detail::any_signal_type::extended_slot_type ext_slot
) {
  { t.connect(sig, std::move(slot)) } -> std::same_as<void>;
  { t.connect_extended(sig, std::move(ext_slot)) } -> std::same_as<void>;
};

///
/// Base of a class with the signals \p T, others connect to them, the class emits them.
///
template<std::default_initializable T>
class adapter
{
  // Variables
  mutable T sigs_; // const members emit and connect too

public: // Typedefs
  using signals_type = T;

public: // Structors
  adapter() = default;
  virtual ~adapter() noexcept = default;

public: // Modifiers
  ///
  /// Connect \p slot to the signal \p sig_projection projects to.
  /// \return the connection, it disconnects when destroyed
  ///
  [[nodiscard]] auto connect(auto sig_projection, auto&& slot) const -> scoped_connection_type;

  ///
  /// Connect the extended \p slot, it gets its connection as first argument, \see connect.
  /// \return the connection
  ///
  [[nodiscard]] auto connect_extended(auto sig_projection, auto&& slot) const -> connection_type;

  ///
  /// Connect \p slot, \p connections queues it instead of the emitting thread calling it. The
  /// connection ends with \p connections.
  ///
  auto connect_queued(auto sig_projection, auto&& slot, connector_kind auto& connections) const
    -> void;

  ///
  /// \see connect_queued, for an extended \p slot
  ///
  auto connect_queued_extended(
    auto sig_projection, auto&& slot, connector_kind auto& connections
  ) const -> void;

protected: // Accessors
  ///
  /// Emit the signal \p sig_projection projects to with \p args.
  /// \return the result of the signal, if it has one
  ///
  auto notify(auto sig_projection, auto&&... args) -> auto;
};

///
///
template<std::default_initializable T>
auto adapter<T>::connect(auto sig_projection, auto&& slot) const -> scoped_connection_type
{
  return std::invoke(sig_projection, sigs_).connect(std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_extended(auto sig_projection, auto&& slot) const -> connection_type
{
  return std::invoke(sig_projection, sigs_).connect_extended(std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_queued(
  auto sig_projection, auto&& slot, connector_kind auto& connections
) const -> void
{
  connections.connect(std::invoke(sig_projection, sigs_), std::forward<decltype(slot)>(slot));
}

///
///
template<std::default_initializable T>
auto adapter<T>::connect_queued_extended(
  auto sig_projection, auto&& slot, connector_kind auto& connections
) const -> void
{
  connections.connect_extended(
    std::invoke(sig_projection, sigs_), std::forward<decltype(slot)>(slot)
  );
}

///
///
template<std::default_initializable T>
auto adapter<T>::notify(auto sig_projection, auto&&... args) -> auto
{
  return std::invoke(sig_projection, sigs_)(std::forward<decltype(args)>(args)...);
}

} // namespace arcstd::signal
