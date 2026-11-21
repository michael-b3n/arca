#include <arcstd/util/timer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

namespace arcstd::util
{

TEST_CASE("timer_measures_the_time_since_its_construction", "[util]")
{
  const auto timer = util::timer{};
  CHECK(timer.current_duration() >= std::chrono::milliseconds{0});
  std::this_thread::sleep_for(std::chrono::milliseconds{20});
  CHECK(timer.current_duration() >= std::chrono::milliseconds{20});
}

TEST_CASE("scoped_timer_logger_logs_without_a_logger", "[util]")
{
  // The message is dropped, there is no logger
  CHECK_NOTHROW([] { SCOPED_TIMER_LOG(); }());
}

} // namespace arcstd::util
