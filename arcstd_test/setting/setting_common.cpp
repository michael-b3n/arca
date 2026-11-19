#include <arcstd/setting/setting_common.hpp>

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

} // namespace arcstd::setting
