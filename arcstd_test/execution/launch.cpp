#include <arcstd/execution/launch.hpp>
#include <arcstd/execution/scope.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <stdexcept>
#include <thread>

namespace arcstd::execution
{

TEST_CASE("launch_runs_work_on_another_thread", "[execution]")
{
  auto thread = std::thread::id{};
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  const auto note_thread = [&]
  {
    thread = std::this_thread::get_id();
    ran = true;
    ran.notify_all();
  };
  launch(note_thread, work);
  ran.wait(false);
  CHECK(thread != std::this_thread::get_id());
}

TEST_CASE("launch_survives_failing_work", "[execution]")
{
  auto ran = std::atomic_bool{false};
  auto work = scope{};
  launch([] { throw std::runtime_error{"expected by the test"}; }, work);
  const auto succeed = [&]
  {
    ran = true;
    ran.notify_all();
  };
  launch(succeed, work);
  ran.wait(false);
  CHECK(ran);
}

} // namespace arcstd::execution
