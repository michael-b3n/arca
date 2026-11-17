#pragma once

#include "arcstd/util/source_location_helpers.hpp"

#include <filesystem>
#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace arcstd::util
{

enum class logger_level
{
  debug,
  info,
  warning,
  error,
};

///
/// \return the lowest level logged, debug only in a debug build
///
[[nodiscard]] constexpr auto global_log_level() -> logger_level
{
#ifdef NDEBUG
  return logger_level::info;
#else
  return logger_level::debug;
#endif
}

///
/// Log \p msg, a message that can not be written is dropped: logging never throws.
///
auto log_debug(std::string_view msg) noexcept -> void;
auto log_info(std::string_view msg) noexcept -> void;
auto log_warn(std::string_view msg) noexcept -> void;
auto log_error(std::string_view msg) noexcept -> void;

///
/// The logger while it lives, a message logged without one is dropped.
///
struct logger final
{
  ///
  /// Log to the folder "logs" of \p data_folder, the data folder of the app. \p header is written
  /// at the top of every log file, rotated ones included.
  ///
  explicit logger(const std::filesystem::path& data_folder, std::string header = {});
  ~logger() noexcept;

  logger(const logger&) = delete;
  logger(logger&&) = delete;
  auto operator=(const logger&) -> logger& = delete;
  auto operator=(logger&&) -> logger& = delete;
};

namespace detail
{

///
/// Log the message \p fmt formats of \p args with \p Level, the LOG_ macros call it. A message that
/// can not be formatted is logged as format error, one that can not be written is dropped.
///
template<logger_level Level, typename... Args>
auto log_formatted(const std::format_string<Args...> fmt, Args&&... args) noexcept -> void
{
  try
  {
    try
    {
      const auto message = std::format(fmt, std::forward<Args>(args)...);
      if constexpr(Level == logger_level::debug)
      {
        log_debug(message);
      }
      else if constexpr(Level == logger_level::info)
      {
        log_info(message);
      }
      else if constexpr(Level == logger_level::warning)
      {
        log_warn(message);
      }
      else if constexpr(Level == logger_level::error)
      {
        log_error(message);
      }
    }
    catch(const std::format_error& exception)
    {
      log_error(std::format("[INTERNAL_FMT_STR] format error: {}.", exception.what()));
    }
  }
  // NOLINTNEXTLINE(bugprone-empty-catch) logging never throws, a message without memory is dropped
  catch(...)
  {
  }
}

} // namespace detail

} // namespace arcstd::util

// clang-format off
#define INT_LOG_INTERNAL_FMT_STR(LEVEL, FMT_STR, ...)                                                                                                                                                                  \
  {                                                                                                                                                                                                                    \
    if(LEVEL >= ::arcstd::util::global_log_level())                                                                                                                                                                    \
    {                                                                                                                                                                                                                  \
      static constexpr std::source_location __log_source_location = std::source_location::current();                                                                                                                   \
      static constexpr auto __log_folder_name  = ::arcstd::util::filter_folder_name(__log_source_location);                                                                                                            \
      static constexpr auto __log_file_name  = ::arcstd::util::filter_file_name(__log_source_location);                                                                                                                \
      ::arcstd::util::detail::log_formatted<LEVEL>("[{}::{}] " FMT_STR, __log_folder_name, __log_file_name __VA_OPT__(,) __VA_ARGS__);                                                                                 \
    }                                                                                                                                                                                                                  \
  }
// clang-format on

///
/// Log the message \p FMT_STR formats, with the folder and the file of the caller in front.
///
// clang-format off
#define LOG_DEBUG(FMT_STR, ...) INT_LOG_INTERNAL_FMT_STR(::arcstd::util::logger_level::debug, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_INFO(FMT_STR, ...)  INT_LOG_INTERNAL_FMT_STR(::arcstd::util::logger_level::info, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_WARN(FMT_STR, ...)  INT_LOG_INTERNAL_FMT_STR(::arcstd::util::logger_level::warning, FMT_STR __VA_OPT__(,) __VA_ARGS__);
#define LOG_ERROR(FMT_STR, ...) INT_LOG_INTERNAL_FMT_STR(::arcstd::util::logger_level::error, FMT_STR __VA_OPT__(,) __VA_ARGS__);
// clang-format on
