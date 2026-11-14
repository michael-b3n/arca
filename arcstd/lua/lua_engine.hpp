#pragma once

#include "arcstd/export.hpp"
#include "arcstd/lua/embedded_scripts.hpp"
#include "arcstd/lua/function_cast.hpp"
#include "arcstd/lua/lock.hpp"
#include "arcstd/lua/registration.hpp"
#include "arcstd/lua/script_table.hpp"
#include "arcstd/lua/setting_functions.hpp"
#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_cast.hpp"
#include "arcstd/setting/setting.hpp"
#include "arcstd/setting/setting_common.hpp"
#include "arcstd/setting/setting_store.hpp"
#include "arcstd/signal/adapter.hpp"
#include "arcstd/signal/common.hpp"
#include "arcstd/util/path.hpp"
#include "arcstd/util/string_literal.hpp"

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
#include <variant>
#include <vector>

namespace arcstd::lua
{

class state_owner;

///
/// Manifest of a function a script offers, defined by who calls it: the static id naming the function,
/// e.g. "scripture.passage", its input and its output, usually a script_table.
///
template<typename M>
concept script_manifest = requires {
  { M::id } -> std::convertible_to<util::path>;
} && value_castable<typename M::input> && value_castable<typename M::output>;

///
/// Settings of the Lua engine.
///
class ARCSTD_EXPORT lua_engine_settings final
{
  // Variables
  const std::shared_ptr<setting::setting_store> setting_store_;

public: // Structors
  lua_engine_settings(std::shared_ptr<setting::setting_store> setting_store);

public: // Constants
  static constexpr auto folder_name = "scripts";

public: // Variables
  const setting::setting_store::setting_non_owning_ptr_type<bool> enabled;
  const std::filesystem::path folder;
};

///
/// Signals emitted by the Lua engine.
///
struct lua_engine_signals final
{
  signal::signal_type<void()> scripts_loaded;
};

///
/// Lua engine, owns the Lua state and loads the bundled and user scripts \see load_scripts.
/// An object registers its functions for scripts last in its constructor and keeps the registration as its last
/// member, \see registration.
/// Signal IDs to connect to:
/// - scripts_loaded: Emitted under the Lua lock once the scripts are loaded, also if none are.
///
class ARCSTD_EXPORT lua_engine final : public signal::adapter<lua_engine_signals>
{
  // Typedefs
  struct script_info final
  {
    std::string description;
    std::set<util::path> functions;
  };

  // Constants
  static constexpr auto setting_node = "setting"; // the node of all settings

  // Variables
  const lua_engine_settings settings_;
  const std::function<std::span<const embedded_script>()> bundled_;
  const std::unique_ptr<state_owner> state_owner_;
  mutable std::mutex mtx_;
  std::map<std::string, script_info> scripts_; // the loaded scripts by name
  std::jthread loader_;                        // last, it joins before the members it uses are destroyed

public: // Typedefs
  using bundled_provider_type = std::remove_const_t<decltype(bundled_)>;
  using script_name_type = decltype(scripts_)::key_type;
  using scripts_type = decltype(scripts_);
  using lock_type = lua::lock;
  using registration_type = registration;
  using embedded_script_type = embedded_script;
  template<util::string_literal Key, typename T>
  using field_type = field<Key, T>;
  template<typename... Fields>
  using table_type = script_table<Fields...>;

public: // Structors
  ///
  /// Construct the engine with its settings in \p setting_store, \p bundled shall provide the compiled scripts
  /// when called.
  ///
  lua_engine(std::shared_ptr<setting::setting_store> setting_store, bundled_provider_type bundled);
  ~lua_engine() noexcept;

public: // Accessors
  ///
  /// \return the settings of the engine
  ///
  [[nodiscard]] auto settings() const -> const lua_engine_settings&;

  ///
  /// \return the lock of the scripts, no script runs on another thread for as long as it lives
  ///
  [[nodiscard]] auto lock() const -> lock_type;

  ///
  /// \return the loaded scripts offering a function for each manifest \p M, all of them without \p M, by name
  ///
  template<script_manifest... M>
  [[nodiscard]] auto scripts() const -> scripts_type;

public: // Modifiers
  ///
  /// Load the bundled scripts and, if enabled, the user scripts outside of the calling thread, and emit
  /// scripts_loaded. Called once the functions for scripts are registered, and again to load the scripts anew: a loading still
  /// running ends after its script first.
  ///
  auto load_scripts() -> void;

  ///
  /// Copy the Lua script \p files into the script folder, replacing a file of the same name. They run once the
  /// scripts are loaded anew, \see load_scripts.
  /// \return number of files copied, the others are logged
  ///
  [[nodiscard]] auto import_scripts(const std::vector<std::filesystem::path>& files) -> std::size_t;

  ///
  /// Register a function for scripts at \p p, e.g. "web.fetch", for as long as the registration is kept.
  /// \p function takes and returns C++ values, \see function_cast. What it reaches must live as long.
  /// \return the registration to keep, one of nothing if \p p can not be registered, the reason is logged
  ///
  template<function_castable F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) const -> registration_type;

  ///
  /// Register \p setting for scripts at setting and its own path, for as long as the registration is
  /// kept, \see setting_functions. \p setting must live as long.
  /// \return the registration to keep, one of nothing if the path can not be registered, the reason is logged
  ///
  template<setting::underlying_setting_type T>
  [[nodiscard]] auto register_setting(setting::setting<T>& setting) const -> registration_type;

  ///
  /// Run the function of the manifest \p M of the script named \p script. Blocks until it returns, it may wait
  /// e.g. for a web page.
  /// \return its output, or std::nullopt if it failed, is missing or its output does not fit \p M
  ///
  template<script_manifest M>
  [[nodiscard]] auto run(const script_name_type& script, const typename M::input& input) const
    -> std::optional<typename M::output>;

  ///
  /// Run \p code as a user script named \p name, in an environment of its own, \see load_scripts. Blocks until it
  /// returns.
  /// \return a copy of what it returns, nil if nothing, or std::nullopt if it failed
  ///
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code) const -> std::optional<value>;

  ///
  /// Stop the scripts with force, no script runs from now on. Called before the app ends, so no object waits
  /// for a script that never returns once it is destroyed, \see state_owner::shutdown
  ///
  auto shutdown() const noexcept -> void;

private: // Implementation
  [[nodiscard]] auto scripts(const std::vector<util::path>& ids) const -> scripts_type;
  [[nodiscard]] auto register_function(const util::path& p, function function) const -> registration_type;
  [[nodiscard]] auto register_functions(const util::path& p, function_table functions) const -> registration_type;
  auto run(
    const script_name_type& script,
    const util::path& id,
    const std::function<value()>& input,
    const std::function<bool(const value&)>& output
  ) const -> void;
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
template<function_castable F>
auto lua_engine::register_function(const util::path& p, F&& function) const -> registration_type
{
  return register_function(p, function_cast(std::forward<F>(function)));
}

///
///
template<setting::underlying_setting_type T>
auto lua_engine::register_setting(setting::setting<T>& setting) const -> registration_type
{
  return register_functions(util::path{setting_node} / util::path{setting.path}, setting_functions(setting));
}

///
///
template<script_manifest M>
auto lua_engine::run(const script_name_type& script, const typename M::input& input) const -> std::optional<typename M::output>
{
  auto output = std::optional<typename M::output>{};
  run(
    script,
    util::path{M::id},
    [&] { return value_cast<typename M::input>::to(input); },
    [&](const value& value)
    {
      output = value_cast<typename M::output>::from(value);
      return output.has_value();
    }
  );
  return output;
}

} // namespace arcstd::lua
