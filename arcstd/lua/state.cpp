#include "arcstd/lua/state.hpp"
#include "arcstd/lua/object_cast.hpp"
#include "arcstd/util/log.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

namespace arcstd::lua
{

///
///
state::state(std::shared_ptr<detail::state_data> data)
  : data_{std::move(data)}
  , lock_{data_->mtx}
{
}

///
///
auto state::operator*() const -> sol::state&
{
  return data_->lua;
}

///
///
auto state::operator->() const -> sol::state*
{
  return &data_->lua;
}

///
///
auto state::register_function(const util::path& p, function function, std::shared_ptr<const void> registered) -> registration
{
  auto parent = free_parent(p);
  if(!parent)
  {
    return {};
  }
  parent->set_function(p.sections().back(), to_object_function(std::move(function)));
  return registration{data_, p, std::move(registered)};
}

///
///
auto state::register_functions(const util::path& p, function_table functions, std::shared_ptr<const void> registered)
  -> registration
{
  auto parent = free_parent(p);
  if(!parent)
  {
    return {};
  }
  auto node = parent->create_named(p.sections().back());
  std::ranges::for_each(
    functions, [&](auto& entry) { node.set_function(entry.first, to_object_function(std::move(entry.second))); }
  );
  return registration{data_, p, std::move(registered)};
}

///
///
auto state::run_script(const std::string_view name, const std::string_view code) -> std::optional<sol::object>
{
  auto& lua = data_->lua;
  const auto environment = sol::environment{lua, sol::create, data_->sandbox};
  return run(
    [&]
    { return lua.safe_script(code, environment, sol::script_pass_on_error, std::format("@{}", name), sol::load_mode::text); }
  );
}

///
///
auto state::free_parent(const util::path& p) -> std::optional<sol::table>
{
  const auto& sections = p.sections();
  if(sections.empty())
  {
    LOG_ERROR("lua registration failed: path is empty");
    return std::nullopt;
  }
  // The table of the last section, missing ones on the way are created. None if the way leads through no table.
  auto parent = std::ranges::fold_left(
    sections | std::views::take(sections.size() - 1),
    std::optional{data_->interface},
    [](std::optional<sol::table> node, const std::string& section) -> std::optional<sol::table>
    {
      if(!node)
      {
        return std::nullopt;
      }
      const sol::object child = (*node)[section];
      if(child.get_type() == sol::type::lua_nil)
      {
        return node->create_named(section);
      }
      return child.get_type() == sol::type::table ? std::optional{child.as<sol::table>()} : std::nullopt;
    }
  );
  if(!parent || (*parent)[sections.back()].valid())
  {
    LOG_ERROR("lua registration failed: path=\"{}\" is taken", p.string());
    return std::nullopt;
  }
  return parent;
}

///
///
auto state::run(const std::function<sol::protected_function_result()>& code) -> std::optional<sol::object>
{
  if(data_->shutdown_flag)
  {
    LOG_ERROR("lua script failed: the state is stopped");
    return std::nullopt;
  }
  const auto result = code();
  if(!result.valid())
  {
    const sol::error error = result;
    LOG_ERROR("lua script failed: {}", error.what());
    return std::nullopt;
  }
  return result.return_count() > 0 ? result.get<sol::object>() : sol::make_object(data_->lua, sol::lua_nil);
}

} // namespace arcstd::lua
