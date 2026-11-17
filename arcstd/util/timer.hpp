#pragma once

#include "arcstd/export.hpp"

#include <boost/preprocessor/cat.hpp>

#include <chrono>
#include <source_location>

namespace arcstd::util
{

///
/// Measures the time since its construction.
///
class ARCSTD_CORE_EXPORT timer final
{
  // Variables
  std::chrono::system_clock::time_point time_point_;

public: // Structors
  timer();

public: // Accessors
  ///
  /// \return the time since the construction
  ///
  [[nodiscard]] auto current_duration() const -> std::chrono::milliseconds;
};

///
/// Logs the time of its life when destroyed, with the function of \p source_location.
///
class ARCSTD_CORE_EXPORT scoped_timer_logger final
{
  // Variables
  const std::source_location source_location_;
  const timer timer_;

public: // Structors
  explicit scoped_timer_logger(
    std::source_location source_location = std::source_location::current()
  );
  ~scoped_timer_logger();

  scoped_timer_logger(const scoped_timer_logger&) = delete;
  scoped_timer_logger(scoped_timer_logger&&) = delete;
  auto operator=(const scoped_timer_logger&) -> scoped_timer_logger& = delete;
  auto operator=(scoped_timer_logger&&) -> scoped_timer_logger& = delete;
};

} // namespace arcstd::util

///
/// Log the time until the end of the scope.
///
#define SCOPED_TIMER_LOG()                                                                         \
  [[maybe_unused]] const auto BOOST_PP_CAT(__scoped_timer_logger_instance, __LINE__) =             \
    ::arcstd::util::scoped_timer_logger();
