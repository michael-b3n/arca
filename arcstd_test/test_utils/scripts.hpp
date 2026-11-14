#pragma once

#include <arcstd/setting/setting_store.hpp>
#include <arcstd/workflow/workflow_script.hpp>

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
/// Settings in \p data_folder with the script settings written before a workflow reads them. The user scripts
/// are the folder "scripts" in it.
/// An absolute folder name replaces the local data folder, so the tests leave it alone.
///
[[nodiscard]] inline auto make_script_settings(const std::filesystem::path& data_folder, const bool enabled = false)
  -> std::shared_ptr<setting::setting_store>
{
  {
    const auto writer = std::make_shared<setting::setting_store>(data_folder.string());
    const auto settings = workflow::workflow_script_settings{writer};
    std::ignore = settings.enabled->value(enabled);
  }
  return std::make_shared<setting::setting_store>(data_folder.string());
}

///
/// Load the scripts of \p workflow and wait for scripts_loaded.
///
inline auto load_scripts(workflow::workflow_script& workflow) -> void
{
  const auto loaded = std::make_shared<std::promise<void>>();
  auto done = loaded->get_future();
  const auto connection =
    workflow.connect(&workflow::workflow_script_sigs::scripts_loaded, [loaded]() { loaded->set_value(); });
  workflow.load_scripts();
  REQUIRE(done.wait_for(std::chrono::seconds{10}) == std::future_status::ready);
}

///
/// \return what \p code returns as T, run as a user script of \p workflow, std::nullopt if it failed or returns no T
///
template<lua::value_castable T>
[[nodiscard]] auto script_value(const workflow::workflow_script& workflow, const std::string& code) -> std::optional<T>
{
  return workflow.run_script("test.lua", code)
    .and_then([](const lua::value& value) { return lua::value_cast<T>::from(value); });
}

///
/// \return no bundled scripts, for a workflow script that loads only the scripts of the test
///
[[nodiscard]] inline auto no_bundled() -> std::span<const workflow::workflow_script::embedded_script_type>
{
  return {};
}

} // namespace arcstd::test_utils
