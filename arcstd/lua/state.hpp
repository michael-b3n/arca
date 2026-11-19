#pragma once

#ifndef ARCSTD_INTERNAL
  #error "private header of arcstd"
#endif

#include "arcstd/lua/function.hpp"
#include "arcstd/lua/registration.hpp"
#include "arcstd/lua/sol.hpp"
#include "arcstd/util/path.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>

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
/// Locked Lua state, handed out by state_owner::lock. The lock is held as long as this object
/// lives. Lua references taken from it (tables, functions, objects) must not outlive it.
///
class state final
{
  // Friends
  friend class state_owner;

  // Variables
  std::shared_ptr<detail::state_data> data_;
  std::unique_lock<std::recursive_mutex> lock_;

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
  /// Register \p function at \p p, the last section is its name, for as long as the registration is
  /// kept. What it reaches must live as long. Once taken back, a script still holding the function
  /// gets an error calling it.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  [[nodiscard]] auto register_function(const util::path& p, function function) -> registration;

  ///
  /// Register \p functions as table at \p p, for as long as the registration is kept, \see
  /// register_function.
  /// \return the registration, one of nothing if \p p can not be registered
  ///
  [[nodiscard]] auto register_functions(const util::path& p, function_table functions)
    -> registration;

  ///
  /// Run a user script in an environment of its own, so its globals do not reach other scripts.
  /// Text only, Lua does not verify bytecode and a damaged one crashes past any error handling.
  /// \return what the script returns, nil if nothing, or std::nullopt if it failed
  ///
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code)
    -> std::optional<sol::object>;

  ///
  /// Call \p function with \p args.
  /// \return its first result, nil if none, or std::nullopt if it failed
  ///
  template<typename... Args>
  [[nodiscard]] auto call(const sol::protected_function& function, Args&&... args)
    -> std::optional<sol::object>;

private: // Structors
  explicit state(std::shared_ptr<detail::state_data> data);

private: // Implementation
  [[nodiscard]] auto free_parent(const util::path& p) -> std::optional<sol::table>;
  [[nodiscard]] auto run(const std::function<sol::protected_function_result()>& code)
    -> std::optional<sol::object>;
};

///
///
template<typename... Args>
auto state::call(const sol::protected_function& function, Args&&... args)
  -> std::optional<sol::object>
{
  return run([&] { return function(std::forward<Args>(args)...); });
}

} // namespace arcstd::lua
