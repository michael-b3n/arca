#include "test_utils/settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/setting.hpp>
#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace arcstd::setting
{
namespace
{

using names_type = std::vector<std::string>;

} // namespace

TEST_CASE("setting_tells_when_its_value_changes", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_tells_when_its_value_changes"};
  auto settings = setting_store{folder.path()};
  const auto count = settings.create_setting("test.count", std::int32_t{1}, "items");
  CHECK(count->path == "test.count");
  CHECK(count->postfix == "items");
  const auto counter = test_utils::setting_signal_counter{*count};

  CHECK(count->value(2));
  CHECK(count->value() == 2);
  CHECK(counter.value_changed() == 1);
  // The same value is no change
  CHECK(count->value(2));
  CHECK(counter.value_changed() == 1);
}

TEST_CASE("setting_keeps_its_value_in_its_range", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_keeps_its_value_in_its_range"};
  auto settings = setting_store{folder.path()};
  const auto range = std::make_shared<setting_validator_range<std::int32_t>>(0, 10);
  // The default is corrected too
  const auto count = settings.create_setting("test.count", std::int32_t{20}, range);
  CHECK(count->value() == 9);
  const auto counter = test_utils::setting_signal_counter{*count};

  CHECK_FALSE(count->value(-1));
  CHECK(count->value() == 0);
  CHECK(counter.value_changed() == 1);
  CHECK_FALSE(count->value(-5));
  CHECK(counter.value_changed() == 1);
  CHECK(count->value(5));
  CHECK(count->value() == 5);
  CHECK(counter.value_changed() == 2);
}

TEST_CASE("setting_takes_the_values_of_its_list", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_takes_the_values_of_its_list"};
  auto settings = setting_store{folder.path()};
  const auto list = std::make_shared<setting_validator_list<std::string>>(names_type{"a", "b"});
  const auto name = settings.create_setting("test.name", std::string{"a"}, list);
  const auto counter = test_utils::setting_signal_counter{*name};

  CHECK_FALSE(name->value("c"));
  CHECK(name->value() == "a");
  CHECK(name->value("b"));
  CHECK(name->value() == "b");
  CHECK(counter.value_changed() == 1);

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
    const auto counter = test_utils::setting_signal_counter{*name};
    REQUIRE(list->available(names_type{"c", "a"}));
    CHECK(name->value() == "a");
    CHECK(counter.value_changed() == 1);
    CHECK(counter.validator_changed() == 1);
    // Still in the list, the value stays
    REQUIRE(list->available(names_type{"a", "d"}));
    CHECK(name->value() == "a");
    CHECK(counter.value_changed() == 1);
    CHECK(counter.validator_changed() == 2);
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

} // namespace arcstd::setting
