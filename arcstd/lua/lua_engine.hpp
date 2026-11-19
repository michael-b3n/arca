#pragma once

#include "arcstd/lua/embedded_script.hpp"
#include "arcstd/lua/function.hpp"
#include "arcstd/lua/registration.hpp"
#include "arcstd/lua/script_lock.hpp"
#include "arcstd/lua/setting_functions.hpp"
#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_cast.hpp"
#include "arcstd/setting/setting.hpp"
#include "arcstd/setting/setting_common.hpp"
#include "arcstd/setting/setting_store.hpp"
#include "arcstd/signal/adapter.hpp"
#include "arcstd/signal/common.hpp"
#include "arcstd/util/log.hpp"
#include "arcstd/util/path.hpp"

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace arcstd::lua
{

class state_owner;

///
/// Manifest of a function a script offers, defined by who calls it: the static id naming the
/// function, e.g. "scripture.passage", its input and its output, usually a script_table.
///
template<typename M>
concept script_manifest = requires {
  { M::id } -> std::convertible_to<util::path>;
} && value_castable<typename M::input> && value_castable<typename M::output>;

///
/// Settings of the Lua engine.
///
class lua_engine_settings final
{
  // Variables
  const std::shared_ptr<setting::setting_store> setting_store_;

public: // Constants
  static constexpr auto folder_name = "scripts";

public: // Structors
  explicit lua_engine_settings(std::shared_ptr<setting::setting_store> setting_store);

public: // Variables
  const setting::setting_store::setting_non_owning_ptr_type<bool> enabled;
  const std::filesystem::path folder;
};

///
/// Signals of the Lua engine, `scripts_loaded` is emitted under the Lua lock once the scripts are
/// loaded, also if there are none.
///
struct lua_engine_signals final
{
  signal::signal_type<void()> scripts_loaded;
};

///
/// Owns the Lua state and runs the bundled and the user scripts, \see load_scripts. An object
/// registers its functions for scripts last in its constructor and keeps the registration as its
/// last member, \see registration.
///
class lua_engine final : public signal::adapter<lua_engine_signals>
{
  // Typedefs
  struct script_info final
  {
    std::string description;
    std::set<util::path> functions;
  };

  // Constants
  static constexpr auto setting_node = "setting";

  // Variables
  const lua_engine_settings settings_;
  const std::function<std::span<const embedded_script>()> bundled_;
  const std::unique_ptr<state_owner> state_owner_;
  mutable std::mutex mtx_;
  std::map<std::string, script_info> scripts_;
  std::jthread loader_; // last, it joins before the members it uses are destroyed

public: // Typedefs
  using bundled_provider_type = std::remove_const_t<decltype(bundled_)>;
  using script_name_type = decltype(scripts_)::key_type;
  using scripts_type = decltype(scripts_);
  using lock_type = script_lock;
  using registration_type = registration;
  using embedded_script_type = embedded_script;

public: // Structors
  ///
  /// Engine with its settings in \p setting_store, \p bundled returns the bundled scripts.
  ///
  lua_engine(std::shared_ptr<setting::setting_store> setting_store, bundled_provider_type bundled);
  ~lua_engine() noexcept override;

public: // Accessors
  [[nodiscard]] auto settings() const -> const lua_engine_settings&;

  ///
  /// \return the lock of the scripts, no script runs on another thread for as long as it lives
  ///
  [[nodiscard]] auto lock() const -> lock_type;

  ///
  /// \return the loaded scripts by name that offer a function of each manifest \p M, all of them
  /// without a manifest
  ///
  template<script_manifest... M>
  [[nodiscard]] auto scripts() const -> scripts_type;

public: // Modifiers
  ///
  /// Load the bundled scripts and, if enabled, the user scripts on a thread of their own, then emit
  /// scripts_loaded. Called once the functions for scripts are registered, and again to reload: a
  /// loading still running stops after its current script first.
  ///
  auto load_scripts() -> void;

  ///
  /// Copy the Lua script \p files into the script folder, replacing a file of the same name. They
  /// run once the scripts are reloaded.
  /// \return the number of files copied, the others are logged
  ///
  [[nodiscard]] auto import_scripts(const std::vector<std::filesystem::path>& files) const
    -> std::size_t;

  ///
  /// Register \p function for scripts at \p p, e.g. "web.fetch", while the registration is kept.
  /// It takes and returns C++ values, \see to_function, what it reaches must live as long.
  /// \return the registration, an empty one if \p p can not be registered, the reason is logged
  ///
  template<function_convertible F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) const
    -> registration_type;

  ///
  /// Register \p setting for scripts at "setting.<its path>" while the registration is kept, \see
  /// setting_functions. \p setting must live as long.
  /// \return the registration, an empty one if the path can not be registered, the reason is logged
  ///
  template<setting::underlying_setting_type T>
  [[nodiscard]] auto register_setting(setting::setting<T>& setting) const -> registration_type;

  ///
  /// Run the function of manifest \p M of the script \p script, blocks until it returns.
  /// \return its output, std::nullopt if it failed, is missing or its output does not fit \p M
  ///
  template<script_manifest M>
  [[nodiscard]] auto run(const script_name_type& script, const typename M::input& input) const
    -> std::optional<typename M::output>;

  ///
  /// Run \p code as a user script named \p name in an environment of its own, blocks until it
  /// returns.
  /// \return a copy of what it returns, nil if nothing, std::nullopt if it failed
  ///
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code) const
    -> std::optional<value>;

  ///
  /// Stop the scripts with force, none runs from now on. Called before the app ends, so no object
  /// waits on its destruction for a script that never returns.
  ///
  auto shutdown() const noexcept -> void;

private: // Implementation
  [[nodiscard]] auto scripts(const std::vector<util::path>& ids) const -> scripts_type;
  [[nodiscard]] auto register_function(const util::path& p, function function) const
    -> registration_type;
  [[nodiscard]] auto register_functions(const util::path& p, function_table functions) const
    -> registration_type;
  [[nodiscard]] auto call(
    const script_name_type& script, const util::path& id, const value& input
  ) const -> std::optional<value>;
  auto load(const std::stop_token& stop_token) -> void;
  auto load_folder(const std::filesystem::path& folder, const std::stop_token& stop_token) -> void;
  auto load_script(std::string_view file, std::string_view code) -> void;
};

///
///
template<script_manifest... M>
auto lua_engine::scripts() const -> scripts_type
{
  return scripts(std::vector<util::path>{util::path{M::id}...});
}

///
///
template<function_convertible F>
auto lua_engine::register_function(const util::path& p, F&& function) const -> registration_type
{
  return register_function(p, to_function(std::forward<F>(function)));
}

///
///
template<setting::underlying_setting_type T>
auto lua_engine::register_setting(setting::setting<T>& setting) const -> registration_type
{
  return register_functions(
    util::path{setting_node} / util::path{setting.path}, setting_functions(setting)
  );
}

///
///
template<script_manifest M>
auto lua_engine::run(const script_name_type& script, const typename M::input& input) const
  -> std::optional<typename M::output>
{
  const auto id = util::path{M::id};
  const auto result = call(script, id, to_value<typename M::input>(input));
  if(!result)
  {
    return std::nullopt;
  }
  auto output = from_value<typename M::output>(*result);
  if(!output)
  {
    LOG_ERROR("lua script output rejected: script=\"{}\", function=\"{}\"", script, id.string());
  }
  return output;
}

} // namespace arcstd::lua
