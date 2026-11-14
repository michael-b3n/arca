#pragma once

#if !defined(ARCSTD_INTERNAL)
  #error "arcstd/lua/state.hpp is a private header of arcstd"
#endif

#include "arcstd/framework/setting.hpp"
#include "arcstd/framework/setting_common.hpp"
#include "arcstd/lua/function_cast.hpp"
#include "arcstd/lua/registration.hpp"
#include "arcstd/lua/setting_cast.hpp"
#include "arcstd/lua/sol.hpp"
#include "arcstd/util/path.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace arcstd::lua
{

class state_owner;

namespace detail
{

///
/// Data of a Lua state, shared by its owners and locks.
///
struct state_data final
{
  std::recursive_mutex mtx;
  std::atomic<bool> shutdown_flag{false};
  sol::state lua;
  sol::table interface; // root.interface, what scripts see
  sol::table sandbox;   // its read-only view, the globals scripts run on
};

} // namespace detail

///
/// Locked Lua state, handed out by state_owner::lock. The lock is held as long as this object lives.
/// Lua references taken from it (tables, functions, objects) must not outlive it.
///
class state final
{
  // Friends
  friend class state_owner;

  // Variables
  std::shared_ptr<detail::state_data> data_;
  std::unique_lock<std::recursive_mutex> lock_;

private: // Structors
  explicit state(std::shared_ptr<detail::state_data> data);

public: // Structors
  state(const state&) = delete;
  state(state&&) noexcept = default;
  auto operator=(const state&) -> state& = delete;
  auto operator=(state&&) noexcept -> state& = delete;
  ~state() noexcept = default;

public: // Accessors
  [[nodiscard]] auto operator*() const -> sol::state&;
  [[nodiscard]] auto operator->() const -> sol::state*;

public: // Operations
  ///
  /// Register a function at \p p, the last section is its name, for as long as the registration is kept.
  /// What the function reaches must live as long, \see registration. \p function takes and returns C++
  /// values, what a script passes and gets is converted, \see function_cast.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  template<function_castable F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) -> registration;

  ///
  /// Register \p function at \p p, the last section is its name, for as long as the registration is kept:
  /// the registration keeps \p registered, which the function watches, \see function_cast.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  [[nodiscard]] auto register_function(const util::path& p, function function, std::shared_ptr<const void> registered)
    -> registration;

  ///
  /// Register \p setting at \p p, as table with the functions get(), set(value) and postfix(), for as long as the
  /// registration is kept. \p setting must live as long.
  /// \return the registration, one of nothing if the path can not be registered
  ///
  template<framework::underlying_setting_type T>
  [[nodiscard]] auto register_setting(const util::path& p, framework::setting<T>& setting) -> registration;

  ///
  /// Register \p functions as table at \p p, for as long as the registration is kept: the registration keeps
  /// \p registered, which the functions watch, \see function_cast.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  [[nodiscard]] auto register_functions(const util::path& p, function_table functions, std::shared_ptr<const void> registered)
    -> registration;

  ///
  /// Run a user script in an environment of its own, so its globals do not reach other scripts.
  /// Text only, Lua does not verify bytecode and a damaged one crashes past any error handling.
  /// \return what the script returns, nil if nothing, or std::nullopt if it failed
  ///
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code) -> std::optional<sol::object>;

  ///
  /// Call \p function with \p args.
  /// \return its first result, nil if none, or std::nullopt if it failed
  ///
  template<typename... Args>
  [[nodiscard]] auto call(const sol::protected_function& function, Args&&... args) -> std::optional<sol::object>;

private: // Implementation
  [[nodiscard]] auto free_parent(const util::path& p) -> std::optional<sol::table>;
  [[nodiscard]] auto run(const std::function<sol::protected_function_result()>& code) -> std::optional<sol::object>;
};

///
///
template<function_castable F>
auto state::register_function(const util::path& p, F&& function) -> registration
{
  auto registered = std::make_shared<const std::monostate>();
  auto cast = function_cast(std::forward<F>(function), registered);
  return register_function(p, std::move(cast), std::move(registered));
}

///
///
template<framework::underlying_setting_type T>
auto state::register_setting(const util::path& p, framework::setting<T>& setting) -> registration
{
  auto registered = std::make_shared<const std::monostate>();
  auto functions = setting_cast(setting, registered);
  return register_functions(p, std::move(functions), std::move(registered));
}

///
///
template<typename... Args>
auto state::call(const sol::protected_function& function, Args&&... args) -> std::optional<sol::object>
{
  return run([&] { return function(std::forward<Args>(args)...); });
}

} // namespace arcstd::lua
