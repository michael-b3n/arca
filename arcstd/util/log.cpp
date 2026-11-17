#include "arcstd/util/log.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <print>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace arcstd::util
{
namespace
{

constexpr auto logger_name = std::string_view{"main"};
constexpr auto latest_log_file_name = std::string_view{"latest.log"};

///
/// \return \p u8string as std::string, which spdlog takes as UTF-8
///
[[nodiscard]] auto to_string(const std::u8string& u8string) -> std::string
{
  return std::string{u8string.begin(), u8string.end()};
}

///
/// \return the current time in CET as file name, e.g. "2026-11-17_09-30-00.000"
///
[[nodiscard]] auto format_current_time_cet() -> std::string
{
  const auto time_point = std::chrono::system_clock::now();
  static const auto* const cet = std::chrono::locate_zone("Etc/GMT-1");
  return std::format(
    "{:%F_%H-%M-%S}",
    std::chrono::zoned_time{cet, std::chrono::floor<std::chrono::milliseconds>(time_point)}
  );
}

///
/// \return the lock of the logger, it is set up and torn down under it
///
[[nodiscard]] auto lock_logger() -> std::scoped_lock<std::mutex>
{
  static std::mutex mtx;
  return std::scoped_lock{mtx};
}

///
/// \return \p level as level of spdlog
///
[[nodiscard]] constexpr auto to_spdlog_level(const logger_level level) -> spdlog::level::level_enum
{
  switch(level)
  {
  case logger_level::debug: return spdlog::level::debug;
  case logger_level::info: return spdlog::level::info;
  case logger_level::warning: return spdlog::level::warn;
  case logger_level::error: return spdlog::level::err;
  }
  return spdlog::level::debug;
}

///
/// Delete the oldest session log files in \p log_directory, such that at most \p max_count remain.
/// \return number of deleted files
///
[[nodiscard]] auto prune_log_files(
  const std::filesystem::path& log_directory, const std::size_t max_count
) -> std::size_t
{
  auto ec = std::error_code{};
  const auto is_session_log = [&](const auto& entry)
  {
    return entry.is_regular_file(ec) && entry.path().extension() == ".log" &&
           entry.path().filename() != latest_log_file_name;
  };
  auto files =
    std::filesystem::directory_iterator{log_directory, ec} | std::views::filter(is_session_log) |
    std::views::transform([&](const auto& entry)
                          { return std::pair{entry.last_write_time(ec), entry.path()}; }) |
    std::ranges::to<std::vector>();
  if(files.size() <= max_count)
  {
    return 0;
  }

  std::ranges::sort(files, std::ranges::greater{}, &decltype(files)::value_type::first);
  // A file that cannot be deleted is retried on the next startup.
  return static_cast<std::size_t>(std::ranges::count_if(
    files | std::views::drop(max_count),
    [&](const auto& file) { return std::filesystem::remove(file.second, ec); }
  ));
}

///
/// Set up the logger in the folder "logs" of \p data_folder, \see logger::logger.
///
auto init_log(const std::filesystem::path& data_folder, std::string header) -> void
{
  static constexpr auto log_dir = std::string_view("logs");
  static constexpr auto log_pattern = std::string_view("[%Y-%m-%d %H:%M:%S] [%L] [%t] %v");
  static constexpr auto max_file_size = std::size_t{5} * 1024 * 1024;
  // A session keeps its file plus this many rotated ones, e.g. `x.log`, `x.1.log`, `x.2.log`.
  static constexpr auto max_rotated_files = std::size_t{2};
  static constexpr auto max_log_files = std::size_t{20};

  const auto log_directory = data_folder / log_dir;
  std::filesystem::create_directories(log_directory);
  const auto deleted_files = prune_log_files(log_directory, max_log_files - 1);

  const auto log_file = log_directory / (format_current_time_cet() + std::string{".log"});
  const auto log_file_str = to_string(log_file.u8string());

  // Rotation reopens the file, so every file starts with the header.
  auto handlers = spdlog::file_event_handlers{};
  handlers.after_open = [header = std::move(header)](const spdlog::filename_t&, std::FILE* file)
  {
    std::print(file, "{}", header);
    std::ignore = std::fflush(file);
  };

  const auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    log_file_str, max_file_size, max_rotated_files, false, handlers
  );
  file_sink->set_pattern(std::string{log_pattern});

  // Mirrors the session file: truncated where it rotates, cleared of the previous session on open.
  const auto latest_file = log_directory / latest_log_file_name;
  const auto latest_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    to_string(latest_file.u8string()), max_file_size, 0, true, handlers
  );
  latest_sink->set_pattern(std::string{log_pattern});

  const auto sinks = std::vector<spdlog::sink_ptr>{file_sink, latest_sink};
  const auto logger =
    std::make_shared<spdlog::logger>(std::string{logger_name}, sinks.begin(), sinks.end());
  logger->set_level(to_spdlog_level(global_log_level()));
  logger->flush_on(spdlog::level::info);
  spdlog::register_logger(logger);

  log_debug(
    std::format("Init logger: file={}, deleted_old_files={}.", log_file_str, deleted_files)
  );
}

///
/// \return the logger, none if it is not set up
///
[[nodiscard]] auto get_logger() -> std::shared_ptr<spdlog::logger>
{
  return spdlog::get(std::string{logger_name});
}

///
/// Log \p msg with \p level, drop it if it can not be written.
///
auto log_message(const spdlog::level::level_enum level, const std::string_view msg) noexcept -> void
{
  try
  {
    const auto lock = lock_logger();
    if(const auto logger = get_logger())
    {
      logger->log(level, msg);
    }
  }
  // NOLINTNEXTLINE(bugprone-empty-catch) logging never throws, a message not written is dropped
  catch(...)
  {
  }
}

} // namespace

///
///
auto log_debug(const std::string_view msg) noexcept -> void
{
  log_message(spdlog::level::debug, msg);
}

///
///
auto log_info(const std::string_view msg) noexcept -> void
{
  log_message(spdlog::level::info, msg);
}

///
///
auto log_warn(const std::string_view msg) noexcept -> void
{
  log_message(spdlog::level::warn, msg);
}

///
///
auto log_error(const std::string_view msg) noexcept -> void
{
  log_message(spdlog::level::err, msg);
}

///
///
logger::logger(const std::filesystem::path& data_folder, std::string header)
{
  init_log(data_folder, std::move(header));
}

///
///
logger::~logger() noexcept
{
  spdlog::shutdown();
}

} // namespace arcstd::util
