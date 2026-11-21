#include "test_utils/registrations.hpp"

#include <arcstd/lua/function.hpp>
#include <arcstd/lua/state_owner.hpp>
#include <arcstd/util/ranges.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

namespace arcstd::lua
{

TEST_CASE("lua_state_owner_sets_up_the_state", "[lua]")
{
  const auto owner = state_owner{};
  const auto s = owner.lock();
  CHECK(s->script("return type(root) == 'table'").get<bool>());
  CHECK(s->script("return string.upper('abc')").get<std::string>() == "ABC");
  CHECK(
    s->script("return io == nil and os == nil and debug == nil and package == nil").get<bool>()
  );
}

TEST_CASE("lua_state_owner_keeps_the_state_alive", "[lua]")
{
  const auto s = state_owner{}.lock();
  CHECK(s->script("return type(root) == 'table'").get<bool>());
}

TEST_CASE("lua_state_owner_keeps_the_state_when_moved", "[lua]")
{
  // NOLINTNEXTLINE(misc-const-correctness) moved from below
  auto owner = state_owner{};
  // NOLINTNEXTLINE(performance-move-const-arg) the move under test is a copy
  const auto moved = std::move(owner);
  // NOLINTNEXTLINE(bugprone-use-after-move) a move is a copy
  owner.lock()->set("value", 1);
  CHECK(moved.lock()->get<int>("value") == 1);
}

TEST_CASE("lua_state_owner_logs_for_the_scripts", "[lua]")
{
  const auto owner = state_owner{};
  auto s = owner.lock();
  // Scripts have no console, their texts go to the log of the app
  CHECK(s.run_script(
    "log.lua",
    "util.log_debug('text', 1, nil, {}) util.log_info('text') util.log_warning() util.log_error(1)"
  ));
}

TEST_CASE("lua_state_owner_copies_share_the_state", "[lua]")
{
  const auto owner = state_owner{};
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization) the copy is under test
  const auto copy = owner;
  owner.lock()->set("value", 42);
  CHECK(copy.lock()->get<int>("value") == 42);
}

TEST_CASE("lua_state_owner_is_locked_from_several_threads", "[lua]")
{
  static constexpr auto thread_count = 8;
  static constexpr auto increments = 1000;

  const auto owner = state_owner{};
  owner.lock()->set("counter", 0);
  {
    const auto increment = [copy = owner]
    {
      std::ranges::for_each(
        util::ranges::index_view_to(increments),
        [&](const auto) { copy.lock()->script("counter = counter + 1"); }
      );
    };
    // Joined at the end of the scope
    const auto threads =
      util::ranges::index_view_to(thread_count) |
      std::views::transform([&](const auto) { return std::jthread{increment}; }) |
      std::ranges::to<std::vector>();
  }
  CHECK(owner.lock()->get<int>("counter") == thread_count * increments);
}

TEST_CASE("lua_state_owner_is_locked_again_from_a_script", "[lua]")
{
  auto owner = state_owner{};
  auto keep = test_utils::registrations{};
  {
    auto s = owner.lock();
    s->set("value", 7);
    // A copy kept by the state itself would keep it alive forever
    CHECK(keep(s.register_function(
      "read_value", to_function([&owner]() { return owner.lock()->get<int>("value"); })
    )));
  }
  CHECK(owner.lock()->script("return root.interface.read_value() * 2").get<int>() == 14);
}

} // namespace arcstd::lua
