#pragma once

#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>

namespace arcstd::test_utils
{

///
/// Settings in \p data_folder with the script settings written before a engine reads them. The user scripts
/// are the folder "scripts" in it.
///
[[nodiscard]] inline auto make_script_settings(const std::filesystem::path& data_folder, const bool enabled = false)
  -> std::shared_ptr<setting::setting_store>
{
  {
    const auto writer = std::make_shared<setting::setting_store>(data_folder);
    const auto settings = lua::lua_engine_settings{writer};
    std::ignore = settings.enabled->value(enabled);
  }
  return std::make_shared<setting::setting_store>(data_folder);
}

///
/// Load the scripts of \p engine and wait for scripts_loaded.
///
inline auto load_scripts(lua::lua_engine& engine) -> void
{
  const auto loaded = std::make_shared<std::promise<void>>();
  const auto done = loaded->get_future();
  const auto connection = engine.connect(&lua::lua_engine_signals::scripts_loaded, [loaded] { loaded->set_value(); });
  engine.load_scripts();
  REQUIRE(done.wait_for(std::chrono::seconds{10}) == std::future_status::ready);
}

///
/// \return what \p code returns as T, run as a user script of \p engine, std::nullopt if it failed or returns no T
///
template<lua::value_castable T>
[[nodiscard]] auto script_value(const lua::lua_engine& engine, const std::string& code) -> std::optional<T>
{
  return engine.run_script("test.lua", code).and_then([](const lua::value& value) { return lua::value_cast<T>::from(value); });
}

///
/// \return no bundled scripts, for a engine script that loads only the scripts of the test
///
[[nodiscard]] inline auto no_bundled() -> std::span<const lua::lua_engine::embedded_script_type>
{
  return {};
}

} // namespace arcstd::test_utils
