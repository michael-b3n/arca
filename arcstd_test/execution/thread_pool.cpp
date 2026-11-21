#include <arcstd/execution/launch.hpp>
#include <arcstd/execution/scope.hpp>
#include <arcstd/execution/thread_pool.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>

namespace arcstd::execution
{

TEST_CASE("thread_pool_starts_anew_after_its_shutdown", "[execution]")
{
  const auto run_once = []
  {
    auto ran = std::atomic_bool{false};
    auto work = scope{};
    launch(
      [&]
      {
        ran = true;
        ran.notify_all();
      },
      work
    );
    ran.wait(false);
  };
  run_once();
  shutdown_thread_pool();
  run_once();
  // Twice is fine, and once without a pool too
  shutdown_thread_pool();
  shutdown_thread_pool();
}

} // namespace arcstd::execution
