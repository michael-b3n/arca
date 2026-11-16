#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/cache/cache_store.hpp>
#include <arcstd/lua/lua_engine.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace arcstd::cache
{

TEST_CASE("cache_store_keeps_values_of_scripts", "[cache]")
{
  const auto folder = test_utils::temp_folder{"cache_store_keeps_values_of_scripts"};
  const auto run = [](const lua::lua_engine& script, const std::string& code)
  { return script.run_script("test.lua", code).has_value(); };
  const auto text = [](const lua::lua_engine& script, const std::string& code)
  { return test_utils::script_value<std::string>(script, code); };
  {
    const auto settings = test_utils::make_script_settings(folder.path());
    const auto script = std::make_shared<lua::lua_engine>(settings, test_utils::no_bundled);
    const auto cache = cache_store{settings->data_folder(), script};
    CHECK(test_utils::script_value<bool>(*script, "return cache.get('a', 'key') == nil") == true);
    CHECK(run(*script, "cache.set('a', 'key', 'value a') cache.set('b', 'key', 'value b')"));
    CHECK(run(*script, "cache.set('a', 'gone', 'x') cache.set('a', 'gone', nil)"));
    // A name reaching out of the folder is made one inside of it
    CHECK(run(*script, "cache.set('../a', 'key', 'x')"));
    CHECK(text(*script, "return cache.get('___a', 'key')") == "x");
    CHECK_FALSE(run(*script, "cache.get('', 'key')"));
    CHECK(run(*script, "cache.set_all('c', { one = '1', two = '2' })"));
    CHECK(text(*script, "return cache.get('c', 'two')") == "2");
    CHECK(run(*script, "cache.clear('c')"));
    CHECK(test_utils::script_value<bool>(*script, "return cache.get('c', 'one') == nil") == true);
  }
  // Kept beyond the run, apart for each name
  const auto settings = test_utils::make_script_settings(folder.path());
  const auto script = std::make_shared<lua::lua_engine>(settings, test_utils::no_bundled);
  auto cache = cache_store{settings->data_folder(), script};
  CHECK(text(*script, "return cache.get('a', 'key')") == "value a");
  CHECK(text(*script, "return cache.get('b', 'key')") == "value b");
  CHECK(test_utils::script_value<bool>(*script, "return cache.get('a', 'gone') == nil") == true);
  CHECK(cache.get(util::identifier{"a"}, "key") == "value a");
  CHECK(std::filesystem::exists(folder.path() / "cache" / "a.sqlite"));
  CHECK(std::filesystem::exists(folder.path() / "cache" / "___a.sqlite"));
  CHECK_FALSE(std::filesystem::exists(folder.path() / "a.sqlite"));
}

TEST_CASE("cache_store_takes_its_functions_back_on_destruction", "[cache]")
{
  const auto folder =
    test_utils::temp_folder{"cache_store_takes_its_functions_back_on_destruction"};
  const auto settings = test_utils::make_script_settings(folder.path());
  const auto script = std::make_shared<lua::lua_engine>(settings, test_utils::no_bundled);
  {
    const auto cache = cache_store{settings->data_folder(), script};
    CHECK(test_utils::script_value<bool>(*script, "return type(cache.get) == 'function'") == true);
  }
  // Only them, the scripts go on
  CHECK(test_utils::script_value<bool>(*script, "return cache == nil") == true);
  CHECK(script->run_script("after.lua", "return 1"));
}

} // namespace arcstd::cache
