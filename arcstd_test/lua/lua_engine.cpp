#include "test_utils/files.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/lua/embedded_script.hpp>
#include <arcstd/lua/lua_engine.hpp>
#include <arcstd/lua/script_table.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace arcstd::lua
{
namespace
{

// Manifest of the tests, its function reports what the script saw while loading
struct report_manifest final
{
  static inline const util::path id{"test.report"};
  using input = script_table<>;
  using output = script_table<field<"value", std::string>>;
};

// Manifests a script offers or not
struct a_manifest final
{
  static inline const util::path id{"test.a"};
  using input = script_table<>;
  using output = script_table<>;
};

struct b_manifest final
{
  static inline const util::path id{"test.b"};
  using input = script_table<>;
  using output = script_table<>;
};

struct missing_manifest final
{
  static inline const util::path id{"test.missing"};
  using input = script_table<>;
  using output = script_table<>;
};

// Echoes its input, each type of value a script can take and return
using echo_table = script_table<
  field<"text", std::string>,
  field<"count", std::int64_t>,
  field<"ratio", double>,
  field<"flag", bool>,
  field<"note", std::optional<std::string>>,
  field<"list", std::vector<std::string>>,
  field<"scores", std::map<std::string, std::int64_t>>>;

struct echo_manifest final
{
  static inline const util::path id{"test.echo"};
  using input = echo_table;
  using output = echo_table;
};

///
/// \return Lua of the script \p name offering the function of report_manifest, it tells the value \p expression
/// had while loading
///
[[nodiscard]] auto report(const std::string& name, const std::string& expression) -> std::string
{
  // The node of the name as the bundled scripts have it, e.g. "bundled"
  auto node = name;
  std::ranges::transform(
    node, node.begin(), [](const unsigned char c) { return std::isalnum(c) != 0 ? static_cast<char>(std::tolower(c)) : '_'; }
  );
  return std::format(
    "local value = tostring({}) return {{name = '{}', description = '', node = '{}', "
    "['test.report'] = function() return {{value = value}} end}}",
    expression,
    name,
    node
  );
}

///
/// \return what the scripts offering report_manifest report, as "value (name)"
///
[[nodiscard]] auto reports(const lua_engine& engine) -> std::vector<std::string>
{
  const auto scripts = engine.scripts<report_manifest>();
  return scripts | std::views::keys |
         std::views::transform(
           [&](const lua_engine::script_name_type& script)
           {
             const auto output = engine.run<report_manifest>(script, {});
             return std::format("{} ({})", output ? output->get<"value">() : std::string{"?"}, script);
           }
         ) |
         std::ranges::to<std::vector>();
}

///
/// \return the bundled scripts of the tests, the way a library compiles them in
///
[[nodiscard]] auto bundled() -> std::span<const embedded_script>
{
  static constexpr auto scripts = std::array{
    embedded_script{.name = "bundled.lua", .code = "return {name = 'Bundled', description = '', node = 'bundled'}"}
  };
  return scripts;
}

///
/// \return the names of \p scripts
///
[[nodiscard]] auto script_names(const lua_engine::scripts_type& scripts) -> std::vector<std::string>
{
  return scripts | std::views::keys | std::ranges::to<std::vector>();
}

///
/// \return the names of the scripts the app ships, the ones loaded without the scripts of the user. \p folder is
/// the one of the test.
///
[[nodiscard]] auto bundled_names(const test_utils::temp_folder& folder) -> std::vector<std::string>
{
  auto engine = lua_engine{test_utils::make_script_settings(folder.path() / "bundled"), bundled};
  test_utils::load_scripts(engine);
  return script_names(engine.scripts());
}

///
/// \return the names of the scripts of \p engine without the bundled ones
///
[[nodiscard]] auto user_names(const lua_engine& engine, const test_utils::temp_folder& folder) -> std::vector<std::string>
{
  const auto bundled = bundled_names(folder);
  return script_names(engine.scripts()) |
         std::views::filter([&](const auto& name) { return !std::ranges::contains(bundled, name); }) |
         std::ranges::to<std::vector>();
}

///
/// Object registering functions for scripts, the registration as its last member.
///
class registering_object final
{
  // Variables
  const std::shared_ptr<lua_engine> lua_engine_;
  const std::string name_;

public: // Structors
  explicit registering_object(std::shared_ptr<lua_engine> lua_engine, std::string name = "test");
  ~registering_object() noexcept;

public: // Variables
  bool registered{false};
  int value{42};

private: // Variables
  // Last, so it is taken back first
  registration registrations_;
};

///
///
registering_object::registering_object(std::shared_ptr<lua_engine> lua_engine, std::string name)
  : lua_engine_{std::move(lua_engine)}
  , name_{std::move(name)}
{
  auto nested = lua_engine_->register_function(name_ + ".nested.value", [this] { return value; });
  auto twice = lua_engine_->register_function(name_ + ".twice", [this] { return 2 * value; });
  registered = nested && twice;
  registrations_ << std::move(nested) << std::move(twice);
}

///
///
registering_object::~registering_object() noexcept = default;

} // namespace

TEST_CASE("lua_engine_sets_up_the_state", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_sets_up_the_state"};
  auto engine = lua_engine{test_utils::make_script_settings(folder.path()), bundled};
  // What a script sees
  CHECK(test_utils::script_value<bool>(engine, "return type(util) == 'table' and root == nil and io == nil") == true);
  // Disabled, so loaded without a scripts folder
  test_utils::load_scripts(engine);
  CHECK_FALSE(std::filesystem::exists(folder.path() / "scripts"));
}

TEST_CASE("lua_engine_registers_an_object", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_registers_an_object"};
  const auto script = std::make_shared<lua_engine>(test_utils::make_script_settings(folder.path()), bundled);
  const auto object = std::make_shared<registering_object>(script);
  REQUIRE(object->registered);
  object->value = 7;
  CHECK(test_utils::script_value<int>(*script, "return test.nested.value() + test.twice()") == 21);
}

TEST_CASE("lua_engine_drops_the_functions_of_a_destroyed_object", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_drops_the_functions_of_a_destroyed_object"};
  const auto script = std::make_shared<lua_engine>(test_utils::make_script_settings(folder.path()), bundled);
  const auto other = registering_object{script, "other"};
  {
    const auto object = registering_object{script};
    REQUIRE(script->run_script("before.lua", "held = test.twice return held()"));
  }
  // Its functions only: no script reaches the destroyed object, the other one and the scripts go on
  CHECK(script->run_script("after.lua", "return test == nil and other.twice() == 84"));
  CHECK_FALSE(script->run_script("after.lua", "return test.twice()"));
  CHECK(script->run_script("after.lua", "return 1"));
}

TEST_CASE("lua_engine_rejects_a_taken_name", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_rejects_a_taken_name"};
  const auto script = std::make_shared<lua_engine>(test_utils::make_script_settings(folder.path()), bundled);
  const auto first = registering_object{script, "shared"};
  const auto second = registering_object{script, "shared"};
  CHECK(first.registered);
  CHECK_FALSE(second.registered);
  CHECK(test_utils::script_value<int>(*script, "return shared.nested.value()") == 42);
}

TEST_CASE("lua_engine_loads_the_user_scripts", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_loads_the_user_scripts"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  // Each script reports what it sees
  test_utils::write_file(scripts / "a.lua", "shared = 'a' " + report("a", "shared"));
  // Globals of other scripts are out of reach
  test_utils::write_file(scripts / "b.lua", report("b", "shared"));
  // Failing scripts leave the others running
  test_utils::write_file(scripts / "c.lua", "error('broken')");
  test_utils::write_file(scripts / "e.lua", report("e", "'ran'"));
  test_utils::write_file(scripts / "f.txt", report("f", "'ran'"));

  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"a (a)", "nil (b)", "ran (e)"});
  // Nor of a script run later
  CHECK(test_utils::script_value<bool>(engine, "return shared == nil") == true);
}

TEST_CASE("lua_engine_imports_scripts_into_its_folder", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_imports_scripts_into_its_folder"};
  const auto source = folder.path() / "source";
  std::filesystem::create_directories(source);
  test_utils::write_file(source / "a.lua", report("a", "'first'"));
  test_utils::write_file(source / "b.txt", report("b", "'ran'"));

  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  CHECK(engine.import_scripts({source / "a.lua", source / "b.txt", source / "gone.lua"}) == 1);
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"first (a)"});

  // A file of the same name is replaced, one of the folder itself is not copied onto itself
  test_utils::write_file(source / "a.lua", report("a", "'second'"));
  CHECK(engine.import_scripts({source / "a.lua"}) == 1);
  CHECK(engine.import_scripts({engine.settings().folder / "a.lua"}) == 0);
  // A function loaded before stays, the file loaded later offering it too is logged
  test_utils::write_file(source / "c.lua", report("a", "'third'"));
  CHECK(engine.import_scripts({source / "c.lua"}) == 1);
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"second (a)"});
}

TEST_CASE("lua_engine_loads_the_scripts_anew", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_loads_the_scripts_anew"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", report("a", "'first'"));
  test_utils::write_file(scripts / "b.lua", report("b", "'b'"));
  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"first (a)", "b (b)"});

  // Changed and removed scripts, e.g. loaded again from the settings
  test_utils::write_file(scripts / "a.lua", report("a", "'second'"));
  std::filesystem::remove(scripts / "b.lua");
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"second (a)"});
}

TEST_CASE("lua_engine_ends_loading_once_destroyed", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_ends_loading_once_destroyed"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", "while true do end");
  test_utils::write_file(scripts / "b.lua", "return {}");

  auto engine = std::make_unique<lua_engine>(test_utils::make_script_settings(folder.path(), true), bundled);
  auto loaded = std::atomic<bool>{false};
  const auto connection = engine->connect(&lua_engine_signals::scripts_loaded, [&] { loaded = true; });
  engine->load_scripts();
  std::this_thread::sleep_for(std::chrono::milliseconds{100});
  REQUIRE_FALSE(loaded);
  // The endless script is stopped and the loader joins
  const auto destroyed = std::async(std::launch::async, [&] { engine.reset(); });
  CHECK(destroyed.wait_for(std::chrono::seconds{5}) == std::future_status::ready);
}

TEST_CASE("lua_engine_loads_no_bytecode", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_loads_no_bytecode"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  {
    const auto engine = lua_engine{test_utils::make_script_settings(folder.path()), bundled};
    const auto bytecode = "return string.dump(function() " + report("a", "'ran'") + " end)";
    const auto dumped = test_utils::script_value<std::string>(engine, bytecode);
    REQUIRE(dumped);
    test_utils::write_file(scripts / "a.lua", *dumped);
  }
  test_utils::write_file(
    scripts / "b.lua", report("b", "load(string.dump(function() end)) == nil and dofile == nil and loadfile == nil")
  );

  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  CHECK(reports(engine) == std::vector<std::string>{"true (b)"});
}

TEST_CASE("lua_engine_creates_a_missing_folder", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_creates_a_missing_folder"};
  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  REQUIRE_FALSE(std::filesystem::exists(folder.path() / "scripts"));
  test_utils::load_scripts(engine);
  // Empty, for the user to put scripts into
  CHECK(std::filesystem::is_empty(folder.path() / "scripts"));
  CHECK(user_names(engine, folder).empty());
}

TEST_CASE("lua_engine_loads_the_bundled_scripts", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_loads_the_bundled_scripts"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(scripts / "a.lua", report("a", "'ran'"));
  // A script of the name of a bundled one adds its functions to it
  test_utils::write_file(scripts / "b.lua", report("Bundled", "'ran'"));

  SECTION("without the scripts of the user")
  {
    auto engine = lua_engine{test_utils::make_script_settings(folder.path(), false), bundled};
    test_utils::load_scripts(engine);
    CHECK(std::ranges::contains(script_names(engine.scripts()), std::string{"Bundled"}));
    CHECK(reports(engine).empty());
  }

  SECTION("before the scripts of the user")
  {
    auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
    test_utils::load_scripts(engine);
    CHECK(user_names(engine, folder) == std::vector<std::string>{"a"});
    CHECK(reports(engine) == std::vector<std::string>{"ran (Bundled)", "ran (a)"});
  }
}

TEST_CASE("lua_engine_offers_the_scripts_of_manifests", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_offers_the_scripts_of_manifests"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(
    scripts / "both.lua",
    "return {name = 'Both', description = '', node = 'both', ['test.a'] = function() end, ['test.b'] = function() end}"
  );
  // Only functions named after a manifest count
  test_utils::write_file(
    scripts / "one.lua",
    "return {name = 'One', description = '', node = 'one', ['test.a'] = function() end, ['test.b'] = 1, function() end}"
  );
  // A script returning no table offers nothing
  test_utils::write_file(scripts / "none.lua", "return 1");

  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  CHECK(script_names(engine.scripts<a_manifest>()) == std::vector<std::string>{"Both", "One"});
  CHECK(script_names(engine.scripts<a_manifest, b_manifest>()) == std::vector<std::string>{"Both"});
  CHECK(engine.scripts<missing_manifest>().empty());
  // All of them without a manifest
  CHECK(user_names(engine, folder) == std::vector<std::string>{"Both", "One"});
}

TEST_CASE("lua_engine_knows_a_script_by_its_name", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_knows_a_script_by_its_name"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  const auto script = [](const std::string& fields, const std::string& value)
  { return "return {" + fields + " ['test.report'] = function() return {value = '" + value + "'} end}"; };
  // The name of the file plays no role, the name may be any text
  test_utils::write_file(scripts / "1.lua", script("name = 'Zeta', description = 'The first', node = 'zeta',", "first"));
  test_utils::write_file(scripts / "2.lua", script("name = 'Alpha page.com', description = '', node = 'alpha',", "ran"));
  // Scripts of the same name are one script: a function it has is kept, another one is added
  test_utils::write_file(
    scripts / "3.lua",
    "return {name = 'Zeta', description = 'The second', node = 'zeta', ['test.report'] = function() return {value = 'second'} "
    "end, "
    "['test.a'] = function() end}"
  );
  // No name
  test_utils::write_file(scripts / "5.lua", script("name = 5, description = '', node = 'five',", "ran"));
  test_utils::write_file(scripts / "6.lua", script("description = '', node = 'six',", "ran"));
  test_utils::write_file(scripts / "8.lua", script("name = '', description = '', node = 'eight',", "ran"));
  // No description, an empty one is one
  test_utils::write_file(scripts / "g.lua", script("name = 'Silent', node = 'silent',", "ran"));
  test_utils::write_file(scripts / "h.lua", script("name = 'Numbered', description = 5, node = 'numbered',", "ran"));
  // No node, no Lua name as node, the node of another script and another node of the same script
  test_utils::write_file(scripts / "9.lua", script("name = 'No node',", "ran"));
  test_utils::write_file(scripts / "a.lua", script("name = 'Dotted', description = '', node = 'a.b',", "ran"));
  test_utils::write_file(scripts / "d.lua", script("name = 'Keyword', description = '', node = 'end',", "ran"));
  test_utils::write_file(scripts / "e.lua", script("name = 'Digit', description = '', node = '1st',", "ran"));
  test_utils::write_file(scripts / "f.lua", script("name = 'Dash', description = '', node = 'a-b',", "ran"));
  test_utils::write_file(scripts / "b.lua", script("name = 'Other', description = '', node = 'zeta',", "ran"));
  test_utils::write_file(scripts / "c.lua", script("name = 'Zeta', description = '', node = 'other',", "ran"));

  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  REQUIRE(user_names(engine, folder) == std::vector<std::string>{"Alpha page.com", "Zeta"});
  CHECK(reports(engine) == std::vector<std::string>{"ran (Alpha page.com)", "first (Zeta)"});
  CHECK(script_names(engine.scripts<a_manifest>()) == std::vector<std::string>{"Zeta"});
  // The first file of a script describes it
  CHECK(engine.scripts().at("Zeta").description == "The first");
  CHECK(engine.scripts().at("Alpha page.com").description.empty());
  // Neither by the name of its file
  CHECK_FALSE(engine.run<report_manifest>("1", {}));
}

TEST_CASE("lua_engine_runs_a_function", "[lua]")
{
  const auto folder = test_utils::temp_folder{"lua_engine_runs_a_function"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  test_utils::write_file(
    scripts / "echo.lua",
    R"(
      return { name = "Echo", description = "", node = "echo",
        ["test.echo"] = function(input)
          local case = input.text
          if case == "broken" then error("broken") end
          if case == "no note" then input.note = nil end
          if case == "integral float" then input.count = 3.0 end
          if case == "fraction" then input.count = 1.5 end
          if case == "unknown key" then input.other = "x" end
          if case == "number in list" then input.list = {"a", 1} end
          if case == "number as key" then input.scores = {2} end
          input.text = "echo " .. case
          input.ratio = input.ratio * 2
          input.flag = not input.flag
          return input
        end,
      }
    )"
  );
  auto engine = lua_engine{test_utils::make_script_settings(folder.path(), true), bundled};
  test_utils::load_scripts(engine);
  const auto input = [](const std::string& text)
  {
    return echo_table{
      text, 1, 0.5, true, std::string{"note"},
          {"a", "b"},
          {{"x", 1}}
    };
  };
  const auto run = [&](const std::string& text) { return engine.run<echo_manifest>("Echo", input(text)); };

  CHECK(
    run("john3") == echo_table{
                      "echo john3", 1, 1.0, false, std::string{"note"},
                          {"a", "b"},
                          {{"x", 1}}
  }
  );
  // nil is no value of an optional
  const auto no_note = run("no note");
  REQUIRE(no_note);
  CHECK(no_note->get<"note">() == std::nullopt);
  // A float is no integer, even of an integral value
  CHECK_FALSE(run("integral float"));
  // Output not fitting the manifest, a failing function
  CHECK_FALSE(run("fraction"));
  CHECK_FALSE(run("unknown key"));
  CHECK_FALSE(run("number in list"));
  CHECK_FALSE(run("number as key"));
  CHECK_FALSE(run("broken"));
  // A missing script or function
  CHECK_FALSE(engine.run<echo_manifest>("Missing", input("john3")));
  CHECK_FALSE(engine.run<missing_manifest>("Echo", {}));
}

} // namespace arcstd::lua
