#include <arcstd/system/filesystem.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

namespace arcstd::system
{

TEST_CASE("filesystem_finds_the_running_executable", "[system]")
{
  const auto location = filesystem::executable_location();
  CHECK(location.is_absolute());
  CHECK(location.stem() == "arcstd_test");
  CHECK(std::filesystem::is_regular_file(location));
  CHECK(filesystem::executable_folder() == location.parent_path());
}

} // namespace arcstd::system
