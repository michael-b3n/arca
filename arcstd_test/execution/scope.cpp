#include <arcstd/execution/launch.hpp>
#include <arcstd/execution/scope.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>

namespace arcstd::execution
{

TEST_CASE("scope_stop_waits_for_running_work", "[execution]")
{
  auto started = std::atomic_bool{false};
  auto finished = std::atomic_bool{false};
  auto work = scope{};
  const auto slow = [&]
  {
    started = true;
    started.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    finished = true;
  };
  launch(slow, work);
  started.wait(false);
  work.stop();
  CHECK(finished);
}

TEST_CASE("scope_drops_work_launched_after_stop", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  work.stop();
  launch([&] { ran = true; }, work);
  std::this_thread::sleep_for(std::chrono::milliseconds{20});
  CHECK_FALSE(ran);
}

} // namespace arcstd::execution
