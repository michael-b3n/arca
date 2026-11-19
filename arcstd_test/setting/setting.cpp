#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/setting.hpp>
#include <arcstd/setting/setting_store.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
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

using names_type = std::vector<std::string>;

enum class color
{
  red,
  green,
  blue,
};

///
/// Counts the signals of a setting.
///
struct signal_counter final
{
  int value_changed{0};
  int validator_changed{0};
  signal::scoped_connection_type value_connection;
  signal::scoped_connection_type validator_connection;

  explicit signal_counter(const signal::adapter<setting_signals>& setting);
};

///
///
signal_counter::signal_counter(const signal::adapter<setting_signals>& setting)
  : value_connection{setting.connect(&setting_signals::value_changed, [this] { ++value_changed; })}
  , validator_connection{
      setting.connect(&setting_signals::validator_changed, [this] { ++validator_changed; })
    }
{
}

} // namespace

TEST_CASE("setting_tells_when_its_value_changes", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_tells_when_its_value_changes"};
  auto settings = setting_store{folder.path()};
  const auto count = settings.create_setting("test.count", std::int32_t{1}, "items");
  CHECK(count->path == "test.count");
  CHECK(count->postfix == "items");
  const auto counter = signal_counter{*count};

  CHECK(count->value(2));
  CHECK(count->value() == 2);
  CHECK(counter.value_changed == 1);
  // The same value is no change
  CHECK(count->value(2));
  CHECK(counter.value_changed == 1);
}

TEST_CASE("setting_keeps_its_value_in_its_range", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_keeps_its_value_in_its_range"};
  auto settings = setting_store{folder.path()};
  const auto range = std::make_shared<setting_validator_range<std::int32_t>>(0, 10);
  // The default is corrected too
  const auto count = settings.create_setting("test.count", std::int32_t{20}, range);
  CHECK(count->value() == 9);
  const auto counter = signal_counter{*count};

  CHECK_FALSE(count->value(-1));
  CHECK(count->value() == 0);
  CHECK(counter.value_changed == 1);
  CHECK_FALSE(count->value(-5));
  CHECK(counter.value_changed == 1);
  CHECK(count->value(5));
  CHECK(count->value() == 5);
  CHECK(counter.value_changed == 2);
}

TEST_CASE("setting_takes_the_values_of_its_list", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_takes_the_values_of_its_list"};
  auto settings = setting_store{folder.path()};
  const auto list = std::make_shared<setting_validator_list<std::string>>(names_type{"a", "b"});
  const auto name = settings.create_setting("test.name", std::string{"a"}, list);
  const auto counter = signal_counter{*name};

  CHECK_FALSE(name->value("c"));
  CHECK(name->value() == "a");
  CHECK(name->value("b"));
  CHECK(name->value() == "b");
  CHECK(counter.value_changed == 1);

  // A list not set up yet takes every value
  const auto any = settings.create_setting(
    "test.any", std::string{"x"}, std::make_shared<setting_validator_list<std::string>>()
  );
  CHECK(any->value("y"));
  CHECK(any->value() == "y");
}

TEST_CASE("setting_validates_its_value_when_its_validator_changes", "[setting]")
{
  const auto folder =
    test_utils::temp_folder{"setting_validates_its_value_when_its_validator_changes"};
  auto settings = setting_store{folder.path()};

  SECTION("a plain value becomes the first of the list")
  {
    const auto list = std::make_shared<setting_validator_list<std::string>>(names_type{"a", "b"});
    const auto name = settings.create_setting("test.name", std::string{"b"}, list);
    const auto counter = signal_counter{*name};
    REQUIRE(list->available(names_type{"c", "a"}));
    CHECK(name->value() == "a");
    CHECK(counter.value_changed == 1);
    CHECK(counter.validator_changed == 1);
    // Still in the list, the value stays
    REQUIRE(list->available(names_type{"a", "d"}));
    CHECK(name->value() == "a");
    CHECK(counter.value_changed == 1);
    CHECK(counter.validator_changed == 2);
  }

  SECTION("an optional loses its value")
  {
    using value_type = std::optional<std::string>;
    const auto list = std::make_shared<setting_validator_list<value_type>>(names_type{"a"});
    const auto name = settings.create_setting("test.maybe", value_type{"a"}, list);
    REQUIRE(list->available(names_type{"b"}));
    CHECK(name->value() == std::nullopt);
  }

  SECTION("a vector loses the values no more in the list")
  {
    const auto list = std::make_shared<setting_validator_list<names_type>>(names_type{"a", "b"});
    const auto names = settings.create_setting("test.names", names_type{"a", "b"}, list);
    REQUIRE(list->available(names_type{"b", "c"}));
    CHECK(names->value() == names_type{"b"});
  }
}

TEST_CASE("setting_store_tells_its_settings", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_store_tells_its_settings"};
  auto settings = setting_store{folder.path()};
  CHECK(settings.data_folder() == folder.path());
  CHECK(setting_store::settings_file_path(folder.path()) == folder.path() / "settings.xml");

  auto created = names_type{};
  const auto connection = settings.connect(
    &setting_store_signals::setting_created,
    [&](const std::string& path) { created.push_back(path); }
  );
  std::ignore = settings.create_setting("test.flag", false);
  std::ignore = settings.create_setting("test.color", color::green);
  CHECK(created == names_type{"test.flag", "test.color"});
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

TEST_CASE("setting_type_erased_sets_the_setting_it_erases", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_type_erased_sets_the_setting_it_erases"};
  auto settings = setting_store{folder.path()};
  const auto one = settings.create_setting("test.color", color::green, "hue");

  const auto erased_variant = settings.type_erased_setting("test.color");
  REQUIRE(erased_variant);
  auto* const erased = std::get<setting_type_erased<std::string>*>(*erased_variant);
  CHECK(erased->path == "test.color");
  CHECK(erased->postfix == "hue");
  CHECK(erased->value() == "green");
  const auto& list =
    std::get<setting_validator_list_type_erased<std::string>::sptr_type>(erased->validator);
  CHECK(list->available() == names_type{"red", "green", "blue"});

  const auto counter = signal_counter{erased->signal_adapter};
  CHECK(erased->value("blue"));
  CHECK(one->value() == color::blue);
  CHECK(counter.value_changed == 1);
  // A name of no value of the enum
  CHECK_THROWS_AS(erased->value("purple"), util::exception);
  CHECK(one->value() == color::blue);
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
  const auto first = settings.type_erased_setting("ui.size", std::int32_t{3}, "px", range);
  auto* const size = std::get<setting_type_erased<std::int32_t>*>(first);
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
  const auto again = settings.type_erased_setting("ui.size", std::int32_t{5});
  CHECK(std::get<setting_type_erased<std::int32_t>*>(again) == size);
  CHECK(size->value() == 9);
  CHECK(created == 1);
  CHECK_THROWS_AS(settings.type_erased_setting("ui.size", std::string{"5"}), util::exception);
}

} // namespace arcstd::setting
