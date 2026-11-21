#include "test_utils/settings.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/setting_store.hpp>
#include <arcstd/setting/setting_type_erased.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
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

} // namespace

TEST_CASE("setting_type_erased_sets_the_setting_it_erases", "[setting]")
{
  const auto folder = test_utils::temp_folder{"setting_type_erased_sets_the_setting_it_erases"};
  auto settings = setting_store{folder.path()};
  const auto one = settings.create_setting("test.color", color::green, "hue");

  // An enum is seen as the name of its value
  auto* const erased = test_utils::type_erased_setting<std::string>(settings, "test.color");
  CHECK(erased->path == "test.color");
  CHECK(erased->postfix == "hue");
  CHECK(erased->value() == "green");
  const auto& list =
    std::get<setting_validator_list_type_erased<std::string>::sptr_type>(erased->validator);
  CHECK(list->available() == std::vector<std::string>{"red", "green", "blue"});

  const auto counter = test_utils::setting_signal_counter{erased->signal_adapter};
  CHECK(erased->value("blue"));
  CHECK(one->value() == color::blue);
  CHECK(counter.value_changed() == 1);
  // A name of no value of the enum
  CHECK_THROWS_AS(erased->value("purple"), util::exception);
  CHECK(one->value() == color::blue);
}

} // namespace arcstd::setting
