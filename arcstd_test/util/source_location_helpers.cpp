#include <arcstd/util/source_location_helpers.hpp>

#include <catch2/catch_test_macros.hpp>

#include <source_location>

namespace arcstd::util
{
namespace
{

///
/// \return the location in this function
///
[[nodiscard]] auto location_in_helper() -> std::source_location
{
  return std::source_location::current();
}

} // namespace

TEST_CASE("source_location_helpers_name_the_function", "[util]")
{
  CHECK(filter_function_name(location_in_helper()) == "location_in_helper");
}

TEST_CASE("source_location_helpers_name_the_file_and_its_folder", "[util]")
{
  static constexpr auto location = std::source_location::current();
  static_assert(filter_file_name(location) == "source_location_helpers");
  static_assert(filter_folder_name(location) == "util");
}

} // namespace arcstd::util
