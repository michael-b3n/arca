#include "arcstd/lua/lua_engine.hpp"
#include "arcstd/lua/names.hpp"
#include "arcstd/lua/object_cast.hpp"
#include "arcstd/lua/sol.hpp"
#include "arcstd/lua/state.hpp"
#include "arcstd/lua/state_owner.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/identifier.hpp"
#include "arcstd/util/log.hpp"
#include "arcstd/util/string.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace arcstd::lua
{
namespace
{

///
/// \return the Lua scripts directly inside \p folder, sorted by name
///
[[nodiscard]] auto lua_files(const std::filesystem::path& folder) -> std::vector<std::filesystem::path>
{
  auto error = std::error_code{};
  const auto is_lua_file = [&](const auto& entry)
  { return entry.is_regular_file(error) && entry.path().extension() == ".lua"; };
  auto files = std::filesystem::directory_iterator{folder, error} | std::views::filter(is_lua_file) |
               std::views::transform([](const auto& entry) { return entry.path(); }) | std::ranges::to<std::vector>();
  std::ranges::sort(files);
  return files;
}

///
/// \return the content of \p file
///
[[nodiscard]] auto read(const std::filesystem::path& file) -> std::optional<std::string>
{
  auto stream = std::ifstream{file, std::ios::binary};
  if(!stream)
  {
    LOG_WARN("failed to open file: path=\"{}\"", file.string());
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

///
/// What the script of a file offers.
///
struct offered_script final
{
  std::string name;
  std::string description;
  std::set<util::path> functions;
};

///
/// Add what the script of \p file \p returned to the script tables of \p state.
/// \return the script and the functions the file offers, std::nullopt if it offers nothing or is rejected
///
[[nodiscard]] auto add_script(const state& state, const std::string& file, const sol::object& returned)
  -> std::optional<offered_script>
{
  if(returned.get_type() != sol::type::table)
  {
    LOG_INFO("lua script offers nothing: file=\"{}\"", file);
    return std::nullopt;
  }
  auto table = returned.as<sol::table>();
  const auto text = [&](const std::string_view key)
  { return value_cast<std::string>::from(to_value(table.get<sol::object>(key))); };
  const auto name = text(names::script_name);
  const auto description = text(names::script_description);
  const auto node = text(names::script_node);
  // Other scripts write scripts.<node>, so it is a Lua name
  static constexpr auto is_lua_name = [](const std::string& s)
  {
    // clang-format off
    static constexpr auto keywords = util::string::to_string_view_array(
      "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if",
      "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
    );
    // clang-format on
    return !s.empty() && (s.front() < '0' || s.front() > '9') && (util::identifier{s} == s) &&
           !std::ranges::contains(keywords, s);
  };
  // The description may be empty
  if(!name || name->empty() || !description || !node || !is_lua_name(*node))
  {
    LOG_ERROR("lua script rejected: file=\"{}\", it returns no name, no description or no node", file);
    return std::nullopt;
  }
  // Other scripts see the node, the app runs the functions of the script from there. Files
  // of the same name are one script with one node, and a node belongs to one script.
  sol::table nodes = state->traverse_get<sol::table>(names::node_root, names::node_interface, names::node_scripts);
  sol::table scripts =
    state->traverse_get<sol::table>(names::node_root, names::node_system)[names::node_scripts].get_or_create<sol::table>();
  const sol::object own = scripts[*name];
  const sol::object taken = nodes[*node];
  if((own.valid() || taken.valid()) && own != taken)
  {
    LOG_ERROR(
      "lua script rejected: file=\"{}\", script=\"{}\", node=\"{}\", the script or the node is another's", file, *name, *node
    );
    return std::nullopt;
  }
  sol::table kept = nodes[*node].get_or_create<sol::table>();
  scripts[*name] = kept;
  auto function_names = std::set<util::path>{};
  table.for_each(
    [&](const sol::object& key, const sol::object& value)
    {
      const auto entry = value_cast<std::string>::from(to_value(key));
      if(!entry || *entry == names::script_name || *entry == names::script_description || *entry == names::script_node)
      {
        return;
      }
      // The first file keeps an entry
      if(kept[*entry].valid())
      {
        LOG_WARN("lua script entry rejected: file=\"{}\", script=\"{}\" offers \"{}\" already", file, *name, *entry);
        return;
      }
      kept[*entry] = value;
      // Manifests are named with a dot, e.g. "scripture.read": a function of such a key is one the app may run
      if(const auto function = util::path{*entry}; value.get_type() == sol::type::function && function.sections().size() > 1)
      {
        function_names.insert(function);
      }
    }
  );
  LOG_INFO(
    "lua script loaded: file=\"{}\", name=\"{}\", node=\"{}\", functions={}",
    file,
    *name,
    *node,
    function_names | std::views::transform(&util::path::string)
  );
  return offered_script{.name = *name, .description = *description, .functions = std::move(function_names)};
}

} // namespace

///
///
lua_engine_settings::lua_engine_settings(std::shared_ptr<setting::setting_store> setting_store)
  : setting_store_{std::move(setting_store)}
  , enabled{setting_store_->create_setting("lua.enabled", true)}
  , folder{setting_store_->data_folder() / folder_name}
{
}

///
///
lua_engine::lua_engine(std::shared_ptr<setting::setting_store> setting_store, bundled_provider_type bundled)
  : settings_{std::move(setting_store)}
  , bundled_{std::move(bundled)}
  , state_owner_{std::make_unique<state_owner>()}
{
}

///
///
lua_engine::~lua_engine() noexcept
{
  // The loading ends after its script, which is stopped as it may run endlessly. The loader joins once destroyed.
  loader_.request_stop();
  shutdown();
  // Waits for the running script
  const auto lock = this->lock();
}

///
///
auto lua_engine::settings() const -> const lua_engine_settings&
{
  return settings_;
}

///
///
auto lua_engine::lock() const -> lock_type
{
  return lock_type{std::make_shared<state>(state_owner_->lock())};
}

///
///
auto lua_engine::load_scripts() -> void
{
  loader_ = std::jthread{[this](const std::stop_token& stop_token) { load(stop_token); }};
}

///
///
auto lua_engine::import_scripts(const std::vector<std::filesystem::path>& files) -> std::size_t
{
  const auto& folder = settings().folder;
  auto error = std::error_code{};
  std::ignore = std::filesystem::create_directories(folder, error);
  const auto copy = [&](const std::filesystem::path& file)
  {
    if(
      file.extension() != ".lua" || !std::filesystem::is_regular_file(file, error) ||
      std::filesystem::equivalent(file.parent_path(), folder, error)
    )
    {
      LOG_WARN("script file not imported: file=\"{}\"", file.generic_string());
      return false;
    }
    const auto target = folder / file.filename();
    const auto replaced = std::filesystem::exists(target, error);
    if(!std::filesystem::copy_file(file, target, std::filesystem::copy_options::overwrite_existing, error))
    {
      LOG_ERROR("failed to copy script file: file=\"{}\", error={}", file.generic_string(), error.message());
      return false;
    }
    LOG_INFO("script file {}: file_name=\"{}\"", replaced ? "replaced" : "copied", file.filename().string());
    return true;
  };
  return static_cast<std::size_t>(std::ranges::count_if(files, copy));
}

///
///
auto lua_engine::run_script(const std::string_view name, const std::string_view code) const -> std::optional<value>
{
  auto state = state_owner_->lock();
  return state.run_script(name, code).transform([](const sol::object& returned) { return to_value(returned); });
}

///
///
auto lua_engine::shutdown() const noexcept -> void
{
  state_owner_->shutdown();
}

///
///
auto lua_engine::scripts(const std::vector<util::path>& ids) const -> scripts_type
{
  const auto offers_all = [&ids](const auto& entry)
  { return std::ranges::all_of(ids, [&](const auto& id) { return entry.second.functions.contains(id); }); };
  const auto lock = std::scoped_lock{mtx_};
  return scripts_ | std::views::filter(offers_all) | std::ranges::to<scripts_type>();
}

///
///
auto lua_engine::register_function(const util::path& p, function function, std::shared_ptr<const void> registered) const
  -> registration_type
{
  return state_owner_->lock().register_function(p, std::move(function), std::move(registered));
}

///
///
auto lua_engine::register_functions(const util::path& p, function_table functions, std::shared_ptr<const void> registered) const
  -> registration_type
{
  return state_owner_->lock().register_functions(p, std::move(functions), std::move(registered));
}

///
///
auto lua_engine::run(
  const script_name_type& script,
  const util::path& id,
  const std::function<value()>& input,
  const std::function<bool(const value&)>& output
) const -> void
{
  auto state = state_owner_->lock();
  const auto function = state->traverse_get<sol::optional<sol::protected_function>>(
    names::node_root, names::node_system, names::node_scripts, script, id.string()
  );
  if(!function)
  {
    return;
  }
  if(const auto result = state.call(*function, to_object(*state, input())); result && !output(to_value(*result)))
  {
    LOG_ERROR("lua script output rejected: script=\"{}\", function=\"{}\"", script, id.string());
  }
}

///
///
auto lua_engine::load(const std::stop_token& stop_token) -> void
{
  // Loaded anew, so a script gone from the folder is gone
  {
    auto state = state_owner_->lock();
    state->traverse_get<sol::table>(names::node_root, names::node_system)[names::node_scripts] = sol::lua_nil;
    state->traverse_get<sol::table>(names::node_root, names::node_interface)[names::node_scripts] = state->create_table();
    const auto lock = std::scoped_lock{mtx_};
    scripts_.clear();
  }
  // Faults of the app only, e.g. a vanishing folder. Either way those waiting learn that loading is over.
  try
  {
    std::ranges::for_each(
      bundled_() | std::views::take_while([&](const auto&) { return !stop_token.stop_requested(); }),
      [this](const auto& script) { load_script(script.name, script.code); }
    );
    if(settings().enabled->value())
    {
      const auto& folder = settings().folder;
      auto error = std::error_code{};
      if(!std::filesystem::exists(folder, error))
      {
        if(!std::filesystem::create_directories(folder, error))
        {
          LOG_ERROR("failed to create script folder: folder=\"{}\", error={}", folder.generic_string(), error.message());
        }
      }
      load_folder(folder, stop_token);
    }
  }
  catch(...)
  {
    LOG_ERROR("exception occurred: {}", util::exception_report());
  }
  const auto state = state_owner_->lock();
  notify(&signals_type::scripts_loaded);
}

///
///
auto lua_engine::load_folder(const std::filesystem::path& folder, const std::stop_token& stop_token) -> void
{
  std::ranges::for_each(
    lua_files(folder) | std::views::take_while([&](const auto&) { return !stop_token.stop_requested(); }),
    [&](const auto& file)
    {
      if(const auto code = read(file))
      {
        load_script(file.filename().string(), *code);
      }
    }
  );
}

///
///
auto lua_engine::load_script(const std::string_view file, const std::string_view code) -> void
{
  auto state = state_owner_->lock();
  const auto returned = state.run_script(file, code);
  if(const auto offered = returned ? add_script(state, std::string{file}, *returned) : std::nullopt)
  {
    const auto lock = std::scoped_lock{mtx_};
    // The first file of a script keeps its description
    auto& script =
      scripts_.try_emplace(offered->name, script_info{.description = offered->description, .functions = {}}).first->second;
    script.functions.insert_range(offered->functions);
  }
}

} // namespace arcstd::lua
