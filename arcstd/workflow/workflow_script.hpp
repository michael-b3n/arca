#pragma once

#include "arcstd/export.hpp"
#include "arcstd/lua/embedded_scripts.hpp"
#include "arcstd/lua/function_cast.hpp"
#include "arcstd/lua/lock.hpp"
#include "arcstd/lua/registration.hpp"
#include "arcstd/lua/script_table.hpp"
#include "arcstd/lua/setting_cast.hpp"
#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_cast.hpp"
#include "arcstd/setting/setting.hpp"
#include "arcstd/setting/setting_common.hpp"
#include "arcstd/setting/setting_store.hpp"
#include "arcstd/signal/adapter.hpp"
#include "arcstd/signal/common.hpp"
#include "arcstd/util/path.hpp"
#include "arcstd/util/string_literal.hpp"
#include "arcstd/workflow/workflow_base.hpp"
#include "arcstd/workflow/workflow_settings_base.hpp"

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
} // namespace arcstd::lua

namespace arcstd::workflow
{

///
/// Manifest of a function a script offers, defined by the workflow calling it: the static id naming the function,
/// e.g. "scripture.passage", its input and its output, usually a lua::script_table.
///
template<typename M>
concept script_manifest = requires {
  { M::id } -> std::convertible_to<util::path>;
} && lua::value_castable<typename M::input> && lua::value_castable<typename M::output>;

///
/// Settings corresponding to workflow script.
///
class ARCSTD_EXPORT workflow_script_settings final : public workflow_settings_base
{
public: // Structors
  workflow_script_settings(std::shared_ptr<setting::setting_store> setting_store);
  ~workflow_script_settings() noexcept override = default;

public: // Constants
  static constexpr auto folder_name = "scripts";

public: // Variables
  const setting_type<bool> enabled;
  const std::filesystem::path folder;
};

///
/// Signals emitted by workflow script.
///
struct workflow_script_sigs final
{
  signal::signal_type<void()> scripts_loaded;
};

///
/// Workflow script, owns the Lua state and loads the bundled and user scripts \see load_scripts.
/// Workflows register below root.interface.workflow last in their constructor and keep the registration as their
/// last member, \see lua::registration.
/// Signal IDs to connect to:
/// - scripts_loaded: Emitted under the Lua lock once the scripts are loaded, also if none are.
///
class ARCSTD_EXPORT workflow_script final
  : public workflow_base<workflow_script_settings>
  , public signal::adapter<workflow_script_sigs>
{
  // Typedefs
  struct script_info final
  {
    std::string description;
    std::set<util::path> functions;
  };

  // Constants
  static constexpr auto lua_node = "workflow";    // below root, the node of all workflow registrations
  static constexpr auto setting_node = "setting"; // below lua_node, the node of all settings

  // Variables
  const std::function<std::span<const lua::embedded_script>()> bundled_;
  const std::unique_ptr<lua::state_owner> state_owner_;
  mutable std::mutex mtx_;
  std::map<std::string, script_info> scripts_; // the loaded scripts by name
  std::jthread loader_;                        // last, it joins before the members it uses are destroyed

public: // Typedefs
  using bundled_provider_type = std::remove_const_t<decltype(bundled_)>;
  using script_name_type = decltype(scripts_)::key_type;
  using scripts_type = decltype(scripts_);
  using lock_type = lua::lock;
  using registration_type = lua::registration;
  using embedded_script_type = lua::embedded_script;
  template<util::string_literal Key, typename T>
  using field_type = lua::field<Key, T>;
  template<typename... Fields>
  using table_type = lua::script_table<Fields...>;

public: // Structors
  ///
  /// Construct the workflow, \p bundled shall provide the compiled scripts when called.
  ///
  workflow_script(std::shared_ptr<setting::setting_store> setting_store, bundled_provider_type bundled);
  ~workflow_script() noexcept override;

public: // Accessors
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
  /// scripts_loaded. Called once the workflows registered, and again to load the scripts anew: a loading still
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
  /// Register a function for scripts at workflow.\p p, e.g. "web.fetch", for as long as the registration is kept.
  /// \p function takes and returns C++ values, \see lua::function_cast. What it reaches must live as long.
  /// \return the registration to keep, one of nothing if \p p can not be registered, the reason is logged
  ///
  template<lua::function_castable F>
  [[nodiscard]] auto register_function(const util::path& p, F&& function) const -> registration_type;

  ///
  /// Register \p setting for scripts at workflow.setting and its own path, for as long as the registration is
  /// kept, \see lua::setting_cast. \p setting must live as long.
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
  [[nodiscard]] auto run_script(std::string_view name, std::string_view code) const -> std::optional<lua::value>;

  ///
  /// Stop the scripts with force, no script runs from now on. Called before the app ends, so no workflow waits
  /// for a script that never returns once it is destroyed, \see lua::state_owner::shutdown
  ///
  auto shutdown() const noexcept -> void;

private: // Implementation
  [[nodiscard]] static auto lua_path(const util::path& p) -> util::path;
  [[nodiscard]] auto scripts(const std::vector<util::path>& ids) const -> scripts_type;
  [[nodiscard]] auto register_function(
    const util::path& p, lua::function function, std::shared_ptr<const void> registered
  ) const -> registration_type;
  [[nodiscard]] auto register_functions(
    const util::path& p, lua::function_table functions, std::shared_ptr<const void> registered
  ) const -> registration_type;
  auto run(
    const script_name_type& script,
    const util::path& id,
    const std::function<lua::value()>& input,
    const std::function<bool(const lua::value&)>& output
  ) const -> void;
  auto load(const std::stop_token& stop_token) -> void;
  auto load_folder(const std::filesystem::path& folder, const std::stop_token& stop_token) -> void;
  auto load_script(std::string_view file, std::string_view code) -> void;
};

///
///
template<script_manifest... M>
auto workflow_script::scripts() const -> scripts_type
{
  return scripts(std::vector<util::path>{util::path{M::id}...});
}

///
///
template<lua::function_castable F>
auto workflow_script::register_function(const util::path& p, F&& function) const -> registration_type
{
  auto registered = std::make_shared<const std::monostate>();
  auto cast = lua::function_cast(std::forward<F>(function), registered);
  return register_function(lua_path(p), std::move(cast), std::move(registered));
}

///
///
template<setting::underlying_setting_type T>
auto workflow_script::register_setting(setting::setting<T>& setting) const -> registration_type
{
  auto registered = std::make_shared<const std::monostate>();
  auto functions = lua::setting_cast(setting, registered);
  return register_functions(
    lua_path(util::path{setting_node} / util::path{setting.path}), std::move(functions), std::move(registered)
  );
}

///
///
template<script_manifest M>
auto workflow_script::run(const script_name_type& script, const typename M::input& input) const
  -> std::optional<typename M::output>
{
  auto output = std::optional<typename M::output>{};
  run(
    script,
    util::path{M::id},
    [&] { return lua::value_cast<typename M::input>::to(input); },
    [&](const lua::value& value)
    {
      output = lua::value_cast<typename M::output>::from(value);
      return output.has_value();
    }
  );
  return output;
}

} // namespace arcstd::workflow
