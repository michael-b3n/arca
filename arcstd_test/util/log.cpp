#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/util/log.hpp>
#include <arcstd/util/ranges.hpp>
#include <arcstd/util/timer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace
{

// Its formatter fails at runtime, which the compile time check of the format string can not see
struct unformattable final
{};

} // namespace

template<>
struct std::formatter<unformattable> : std::formatter<std::string_view>
{
  [[nodiscard]] static auto format(const unformattable& value, std::format_context& ctx)
    -> std::format_context::iterator;
};

///
///
auto std::formatter<unformattable>::format(
  [[maybe_unused]] const unformattable& value, [[maybe_unused]] std::format_context& ctx
) -> std::format_context::iterator
{
  throw std::format_error{"unformattable"};
}

namespace arcstd::util
{
namespace
{

///
/// \return the log files in \p log_folder, of the sessions and the latest one
///
[[nodiscard]] auto log_files(const std::filesystem::path& log_folder)
  -> std::vector<std::filesystem::path>
{
  return std::filesystem::directory_iterator{log_folder} |
         std::views::transform([](const auto& entry) { return entry.path(); }) |
         std::views::filter([](const auto& path) { return path.extension() == ".log"; }) |
         std::ranges::to<std::vector>();
}

} // namespace

TEST_CASE("log_drops_messages_without_a_logger", "[util]")
{
  LOG_ERROR("dropped {}", 1);
  CHECK_NOTHROW(log_info("dropped"));
}

TEST_CASE("logger_writes_the_session_and_the_latest_log_file", "[util]")
{
  const auto folder = test_utils::temp_folder{"logger_writes_the_session_and_the_latest_log_file"};
  {
    const auto logger = util::logger{folder.path(), "header\n"};
    LOG_DEBUG("debug {}", 1);
    LOG_INFO("info {}", 2);
    LOG_WARN("warning {}", 3);
    LOG_ERROR("error {}", 4);
    log_info("plain");
    LOG_INFO("{}", unformattable{});
    {
      SCOPED_TIMER_LOG();
    }
  }
  const auto log_folder = folder.path() / "logs";
  const auto files = log_files(log_folder);
  REQUIRE(files.size() == 2);
  const auto latest = test_utils::read_file(log_folder / "latest.log");
  for(const auto& file : files)
  {
    CHECK(test_utils::read_file(file) == latest);
  }

  CHECK(latest.starts_with("header\n"));
  CHECK(latest.contains("[util::log] info 2"));
  CHECK(latest.contains("[util::log] warning 3"));
  CHECK(latest.contains("[util::log] error 4"));
  CHECK(latest.contains("plain"));
  CHECK(latest.contains("[INTERNAL_FMT_STR] format error: unformattable."));
  const auto logs_debug = global_log_level() == logger_level::debug;
  CHECK(latest.contains("[util::log] debug 1") == logs_debug);
  CHECK(latest.contains("measured scoped time duration") == logs_debug);
}

TEST_CASE("logger_keeps_the_newest_session_log_files", "[util]")
{
  const auto folder = test_utils::temp_folder{"logger_keeps_the_newest_session_log_files"};
  const auto log_folder = folder.path() / "logs";
  std::filesystem::create_directories(log_folder);
  const auto now = std::filesystem::file_time_type::clock::now();
  for(const auto age : ranges::index_view_to(25))
  {
    const auto file = log_folder / std::format("old_{:02}.log", age);
    test_utils::write_file(file, "old");
    std::filesystem::last_write_time(file, now - std::chrono::hours{age + 1});
  }
  test_utils::write_file(log_folder / "notes.txt", "kept");
  {
    const auto logger = util::logger{folder.path()};
  }
  // 19 old ones and the one of this session, beside the latest log file
  CHECK(log_files(log_folder).size() == 21);
  CHECK(std::filesystem::exists(log_folder / "old_18.log"));
  CHECK_FALSE(std::filesystem::exists(log_folder / "old_19.log"));
  CHECK(std::filesystem::exists(log_folder / "notes.txt"));
}

} // namespace arcstd::util
