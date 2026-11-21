#include <arcstd/setting/setting_common.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

namespace arcstd::setting
{
namespace
{

enum class shape
{
  circle,
  square,
};

} // namespace

TEST_CASE("setting_enum_converts_at_compile_time", "[setting]")
{
  STATIC_CHECK(create_setting_value_converter<shape, std::string>()(shape::square) == "square");
  STATIC_CHECK(try_from_type_erased_setting<shape>(std::string{"circle"}) == shape::circle);
  STATIC_CHECK(!try_from_type_erased_setting<shape>(std::string{"line"}).has_value());
  STATIC_CHECK(
    try_from_type_erased_setting<std::vector<shape>>(std::vector<std::string>{"square"}) ==
    std::vector{shape::square}
  );
}

TEST_CASE("setting_enum_converts_optional_and_vector_values", "[setting]")
{
  using shapes = std::vector<shape>;
  using names = std::vector<std::string>;

  const auto from_optional =
    create_setting_value_converter<std::optional<std::string>, std::optional<shape>>();
  CHECK(from_optional(std::optional<std::string>{"square"}) == shape::square);
  CHECK(from_optional(std::nullopt) == std::nullopt);
  CHECK_THROWS_AS(from_optional(std::optional<std::string>{"line"}), util::exception);

  const auto from_vector = create_setting_value_converter<names, shapes>();
  CHECK(from_vector(names{"circle", "square"}) == shapes{shape::circle, shape::square});
  CHECK_THROWS_AS(from_vector(names{"circle", "line"}), util::exception);
  CHECK(create_setting_value_converter<shapes, names>()(shapes{shape::circle}) == names{"circle"});

  // Tried, a name of no value of the enum gives none
  using optional_shape = std::optional<shape>;
  CHECK(
    try_from_type_erased_setting<optional_shape>(std::optional<std::string>{}) ==
    std::optional<optional_shape>{optional_shape{}}
  );
  CHECK(
    try_from_type_erased_setting<optional_shape>(std::optional<std::string>{"line"}) == std::nullopt
  );
  CHECK(try_from_type_erased_setting<shapes>(names{"circle", "line"}) == std::nullopt);
}

} // namespace arcstd::setting
