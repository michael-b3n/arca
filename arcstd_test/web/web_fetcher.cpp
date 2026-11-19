#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/web/web_fetcher.hpp>

#include <catch2/catch_test_macros.hpp>

#include <expected>
#include <memory>
#include <string>
#include <tuple>

namespace arcstd::web
{

TEST_CASE("web_fetcher_fetches_web_pages", "[web]")
{
  const auto folder = test_utils::temp_folder{"web_fetcher_fetches_web_pages"};
  const auto settings = test_utils::make_script_settings(folder.path());
  const auto script = std::make_shared<lua::lua_engine>(settings, test_utils::no_bundled);
  const auto web = web_fetcher{settings, script};
  // Fails before any request, so no network is needed, and the same when answered from memory
  CHECK(web.fetch("not a url") == std::unexpected{std::string{"invalid_url"}});
  CHECK(web.fetch("not a url") == std::unexpected{std::string{"invalid_url"}});
  CHECK(
    test_utils::script_value<std::string>(
      *script, "local content, error = web.fetch('not a url') return content == nil and error"
    ) == "invalid_url"
  );
}

TEST_CASE("web_fetcher_keeps_the_retry_pause_in_its_range", "[web]")
{
  using namespace std::chrono_literals;
  const auto folder = test_utils::temp_folder{"web_fetcher_keeps_the_retry_pause_in_its_range"};
  const auto settings = web_fetcher_settings{test_utils::make_script_settings(folder.path())};
  CHECK(settings.retry_pause->value() == 60s);
  // A duration is validated like a number: the range ends before its upper bound. 0 s is no pause.
  std::ignore = settings.retry_pause->value(-1s);
  CHECK(settings.retry_pause->value() == 0s);
  std::ignore = settings.retry_pause->value(48h);
  CHECK(settings.retry_pause->value() == 24h - 1s);
  std::ignore = settings.retry_pause->value(5min);
  CHECK(settings.retry_pause->value() == 5min);
}

TEST_CASE("web_fetcher_takes_its_function_back_on_destruction", "[web]")
{
  const auto folder = test_utils::temp_folder{"web_fetcher_takes_its_function_back_on_destruction"};
  const auto settings = test_utils::make_script_settings(folder.path());
  const auto script = std::make_shared<lua::lua_engine>(settings, test_utils::no_bundled);
  {
    const auto web = web_fetcher{settings, script};
    CHECK(test_utils::script_value<bool>(*script, "return type(web.fetch) == 'function'") == true);
  }
  // Only it, the scripts go on
  CHECK(test_utils::script_value<bool>(*script, "return web == nil") == true);
  CHECK(script->run_script("after.lua", "return 1"));
}

} // namespace arcstd::web
