#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace arcstd::setting
{
namespace
{

enum class color
{
  red,
  green,
  blue,
};

// Shares "red" with color, a stored "green" names none of its values
enum class other_color
{
  red,
  yellow,
};

} // namespace

TEST_CASE("setting_store_keeps_values_beyond_the_run", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_store_keeps_values_beyond_the_run"};
  using colors = std::vector<color>;
  {
    auto settings = setting_store{folder.path().string()};
    // Created with the default, then changed
    const auto flag = settings.create_setting("test.flag", false);
    const auto maybe = settings.create_setting("test.maybe", std::optional<bool>{});
    const auto count = settings.create_setting("test.count", std::int32_t{1});
    const auto name = settings.create_setting("test.name", std::string{"default"});
    const auto pause = settings.create_setting("test.pause", std::chrono::seconds{1});
    const auto path = settings.create_setting("test.path", std::filesystem::path{"a"});
    const auto one = settings.create_setting("test.one", color::red);
    const auto some = settings.create_setting("test.some", std::optional<color>{});
    const auto list = settings.create_setting("test.list", colors{});
    const auto unchanged = settings.create_setting("test.unchanged", std::int32_t{5});
    CHECK(flag->value() == false);
    CHECK(maybe->value() == std::nullopt);
    CHECK(unchanged->value() == 5);
    CHECK(flag->value(true));
    CHECK(maybe->value(false));
    CHECK(count->value(2));
    CHECK(name->value("changed"));
    CHECK(pause->value(std::chrono::seconds{3}));
    CHECK(path->value(std::filesystem::path{"b/c"}));
    CHECK(one->value(color::green));
    CHECK(some->value(color::blue));
    CHECK(list->value(colors{color::blue, color::red}));
  }
  CHECK(std::filesystem::exists(folder.path() / setting_store::settings_file_name));
  // The stored values win over other defaults
  auto settings = setting_store{folder.path().string()};
  CHECK(settings.create_setting("test.flag", false)->value() == true);
  CHECK(settings.create_setting("test.maybe", std::optional<bool>{true})->value() == std::optional{false});
  CHECK(settings.create_setting("test.count", std::int32_t{7})->value() == 2);
  CHECK(settings.create_setting("test.name", std::string{"other"})->value() == "changed");
  CHECK(settings.create_setting("test.pause", std::chrono::seconds{9})->value() == std::chrono::seconds{3});
  CHECK(settings.create_setting("test.path", std::filesystem::path{"x"})->value() == std::filesystem::path{"b/c"});
  CHECK(settings.create_setting("test.one", color::blue)->value() == color::green);
  CHECK(settings.create_setting("test.some", std::optional<color>{})->value() == std::optional{color::blue});
  CHECK(settings.create_setting("test.list", colors{})->value() == colors{color::blue, color::red});
  CHECK(settings.create_setting("test.unchanged", std::int32_t{6})->value() == 5);
}

TEST_CASE("setting_store_takes_the_default_for_a_stored_value_that_does_not_fit", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_store_takes_the_default_for_a_stored_value_that_does_not_fit"};
  {
    auto settings = setting_store{folder.path().string()};
    std::ignore = settings.create_setting("test.one", color::green);
    std::ignore = settings.create_setting("test.same", color::red);
    std::ignore = settings.create_setting("test.list", std::vector<color>{color::red, color::green});
    std::ignore = settings.create_setting("test.some", std::optional{color::green});
    std::ignore = settings.create_setting("test.text", std::string{"text"});
  }
  auto settings = setting_store{folder.path().string()};
  // A name of no value of the enum, also as one of a list
  CHECK(settings.create_setting("test.one", other_color::yellow)->value() == other_color::yellow);
  CHECK(settings.create_setting("test.same", other_color::yellow)->value() == other_color::red);
  CHECK(
    settings.create_setting("test.list", std::vector<other_color>{other_color::yellow})->value() ==
    std::vector<other_color>{other_color::yellow}
  );
  CHECK(settings.create_setting("test.some", std::optional<other_color>{})->value() == std::nullopt);
  // A value of another type
  CHECK(settings.create_setting("test.text", std::int32_t{4})->value() == 4);
}

} // namespace arcstd::setting
