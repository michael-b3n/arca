#pragma once

#include <format>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string>

namespace arcstd::util
{

///
/// Exception of arcstd, it keeps where it was thrown and the stack trace.
///
class exception : public std::runtime_error
{
public: // Structors
  explicit exception(
    const std::string& error,
    const std::source_location& loc = std::source_location::current(),
    std::stacktrace stacktrace = std::stacktrace::current()
  );

public: // Accessors
  [[nodiscard]] auto where() const -> const std::source_location&;
  [[nodiscard]] auto stack() const -> const std::stacktrace&;

protected: // Variables
  const std::source_location location_;
  const std::stacktrace stacktrace_;
};

///
/// \return the report of the exception being handled: its message and, of an util::exception, its
/// location and stack trace
///
[[nodiscard]] auto exception_report() -> std::string;

} // namespace arcstd::util

///
///
template<>
struct std::formatter<arcstd::util::exception> : std::formatter<std::string>
{
  auto format(const arcstd::util::exception& ex, std::format_context& ctx) const
    -> std::format_context::iterator;
};

///
///
inline auto std::formatter<arcstd::util::exception>::format(
  const arcstd::util::exception& ex, std::format_context& ctx
) const -> std::format_context::iterator
{
  const auto* msg = ex.what();
  decltype(auto) loc = ex.where();
  decltype(auto) stack = ex.stack();
  return formatter<std::string>::format(
    std::format(
      "{}\n  file: {}:{}:{}\n  function: {}\n  stack: {}",
      msg,
      loc.file_name(),
      loc.line(),
      loc.column(),
      loc.function_name(),
      stack
    ),
    ctx
  );
}
