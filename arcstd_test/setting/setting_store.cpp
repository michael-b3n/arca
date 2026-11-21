#include "test_utils/settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/setting_store.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
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
    auto settings = setting_store{folder.path()};
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
  auto settings = setting_store{folder.path()};
  CHECK(settings.create_setting("test.flag", false)->value() == true);
  CHECK(
    settings.create_setting("test.maybe", std::optional<bool>{true})->value() ==
    std::optional{false}
  );
  CHECK(settings.create_setting("test.count", std::int32_t{7})->value() == 2);
  CHECK(settings.create_setting("test.name", std::string{"other"})->value() == "changed");
  CHECK(
    settings.create_setting("test.pause", std::chrono::seconds{9})->value() ==
    std::chrono::seconds{3}
  );
  CHECK(
    settings.create_setting("test.path", std::filesystem::path{"x"})->value() ==
    std::filesystem::path{"b/c"}
  );
  CHECK(settings.create_setting("test.one", color::blue)->value() == color::green);
  CHECK(
    settings.create_setting("test.some", std::optional<color>{})->value() ==
    std::optional{color::blue}
  );
  CHECK(settings.create_setting("test.list", colors{})->value() == colors{color::blue, color::red});
  CHECK(settings.create_setting("test.unchanged", std::int32_t{6})->value() == 5);
}

TEST_CASE("setting_store_takes_the_default_for_a_stored_value_that_does_not_fit", "[setting]")
{
  const auto folder =
    test_utils::temp_folder{"setting_store_takes_the_default_for_a_stored_value_that_does_not_fit"};
  {
    auto settings = setting_store{folder.path()};
    std::ignore = settings.create_setting("test.one", color::green);
    std::ignore = settings.create_setting("test.same", color::red);
    std::ignore =
      settings.create_setting("test.list", std::vector<color>{color::red, color::green});
    std::ignore = settings.create_setting("test.some", std::optional{color::green});
    std::ignore = settings.create_setting("test.text", std::string{"text"});
  }
  auto settings = setting_store{folder.path()};
  // A name of no value of the enum, also as one of a list
  CHECK(settings.create_setting("test.one", other_color::yellow)->value() == other_color::yellow);
  CHECK(settings.create_setting("test.same", other_color::yellow)->value() == other_color::red);
  CHECK(
    settings.create_setting("test.list", std::vector<other_color>{other_color::yellow})->value() ==
    std::vector<other_color>{other_color::yellow}
  );
  CHECK(
    settings.create_setting("test.some", std::optional<other_color>{})->value() == std::nullopt
  );
  // A value of another type
  CHECK(settings.create_setting("test.text", std::int32_t{4})->value() == 4);
}

TEST_CASE("setting_store_tells_its_settings", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_store_tells_its_settings"};
  auto settings = setting_store{folder.path()};
  CHECK(settings.data_folder() == folder.path());
  CHECK(
    setting_store::settings_file_path(folder.path()) ==
    folder.path() / setting_store::settings_file_name
  );

  auto created = std::vector<std::string>{};
  const auto connection = settings.connect(
    &setting_store_signals::setting_created,
    [&](const std::string& path) { created.push_back(path); }
  );
  std::ignore = settings.create_setting("test.flag", false);
  std::ignore = settings.create_setting("test.color", color::green);
  CHECK(created == std::vector<std::string>{"test.flag", "test.color"});
  CHECK_THROWS_AS(settings.create_setting("test.flag", true), util::exception);
  CHECK(created.size() == 2);

  const auto all = settings.type_erased_settings();
  REQUIRE(all.size() == 2);
  CHECK(all.at(0).path == "test.flag");
  CHECK(std::holds_alternative<setting_type_erased<bool>*>(all.at(0).setting));
  // An enum is seen as the name of its value
  CHECK(std::holds_alternative<setting_type_erased<std::string>*>(all.at(1).setting));
  CHECK(settings.type_erased_setting("test.missing") == std::nullopt);
  CHECK(settings.type_erased_setting("test.color") == all.at(1).setting);
}

TEST_CASE("setting_store_creates_a_type_erased_setting_once", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_store_creates_a_type_erased_setting_once"};
  auto settings = setting_store{folder.path()};
  auto created = 0;
  const auto connection = settings.connect(
    &setting_store_signals::setting_created, [&](const std::string&) { ++created; }
  );

  const auto range = std::make_shared<setting_validator_range<std::int32_t>>(0, 10);
  auto* const size = test_utils::type_erased_setting<std::int32_t>(
    settings.type_erased_setting("ui.size", std::int32_t{3}, "px", range)
  );
  CHECK(size->value() == 3);
  CHECK(size->postfix == "px");
  CHECK(
    std::holds_alternative<setting_validator_range_type_erased<std::int32_t>::sptr_type>(
      size->validator
    )
  );
  CHECK_FALSE(size->value(12));
  CHECK(size->value() == 9);

  // The existing one, its value stays
  CHECK(
    test_utils::type_erased_setting<std::int32_t>(
      settings.type_erased_setting("ui.size", std::int32_t{5})
    ) == size
  );
  CHECK(size->value() == 9);
  CHECK(created == 1);
  CHECK_THROWS_AS(settings.type_erased_setting("ui.size", std::string{"5"}), util::exception);
}

} // namespace arcstd::setting
