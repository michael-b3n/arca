#pragma once

#include "arcstd/execution/launch.hpp"
#include "arcstd/execution/scope.hpp"
#include "arcstd/framework/process_params.hpp"
#include "arcstd/signal/adapter.hpp"
#include "arcstd/signal/common.hpp"
#include "arcstd/signal/queued_connections.hpp"
#include "arcstd/workflow/workflow_settings.hpp"
#include "arcstd/workflow/workflow_settings_base.hpp"

#include <concepts>
#include <expected>
#include <memory>
#include <type_traits>
#include <utility>

namespace arcstd::workflow
{

///
/// Base class for workflows, also naming the types to work with them.
///
struct workflow_ground
{
  // Typedefs
  using scope_type = execution::scope;
  using connections_type = signal::queued_connections;
  using process_id_type = framework::process_id_type;
  using result_unexpected_type = framework::process_result_unexpected;
  template<typename T>
  using params_type = framework::process_params<T>;
  template<typename T>
  using result_type = framework::process_result<T>;
  template<typename T>
  using signal_type = signal::signal_type<T>;
  template<typename T>
  using signal_adapter_type = signal::adapter<T>;

  // Constants
  static constexpr std::unexpected return_failure{framework::process_result_unexpected::failure};
  static constexpr std::unexpected return_stopped{framework::process_result_unexpected::stopped};

  // Static interface
  ///
  /// Run \p work on the thread pool, detached in \p scope which waits for it, \see execution::launch
  ///
  template<std::invocable F>
    requires std::is_void_v<std::invoke_result_t<F>>
  static auto launch(F&& work, scope_type& scope) -> void;

  // Structors
  virtual ~workflow_ground() noexcept = default;
};

///
///
template<std::invocable F>
  requires std::is_void_v<std::invoke_result_t<F>>
auto workflow_ground::launch(F&& work, scope_type& scope) -> void
{
  execution::launch(std::forward<F>(work), scope);
}

template<typename T>
class workflow_base;

///
/// template specialization \see workflow_base
///
template<>
class workflow_base<void> : public workflow_ground
{
protected: // Structors
  workflow_base() = default;
  ~workflow_base() noexcept override = default;
};

///
/// template specialization \see workflow_base
///
template<std::derived_from<workflow_settings_base> S>
class workflow_base<S> : public workflow_ground
{
protected: // Structors
  workflow_base(std::shared_ptr<workflow_settings> workflow_settings);
  ~workflow_base() noexcept override = default;

public: // Typedefs
  using settings_type = std::conditional_t<std::is_void_v<S>, void, std::unique_ptr<S>>;

public: // Accessors
  ///
  /// \return const reference to settings container
  ///
  [[nodiscard]] auto settings() const -> const S&;

private: // Variables
  const settings_type settings_;
};

///
///
template<std::derived_from<workflow_settings_base> S>
workflow_base<S>::workflow_base(std::shared_ptr<workflow_settings> workflow_settings)
  : settings_{std::make_unique<S>(std::move(workflow_settings))}
{
}

///
///
template<std::derived_from<workflow_settings_base> S>
auto workflow_base<S>::settings() const -> const S&
{
  return *settings_;
}

} // namespace arcstd::workflow
