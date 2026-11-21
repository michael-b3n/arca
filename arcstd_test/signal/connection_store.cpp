#include <arcstd/signal/common.hpp>
#include <arcstd/signal/connection_store.hpp>

#include <catch2/catch_test_macros.hpp>

namespace arcstd::signal
{

TEST_CASE("connection_store_disconnects_its_connections", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  {
    auto store = connection_store{};
    store.store(scoped_connection_type{sig.connect([&count] { ++count; })});
    store.store(scoped_connection_type{sig.connect([&count] { ++count; })});

    sig();
    CHECK(count == 2);
  }

  sig();
  CHECK(count == 2);
}

TEST_CASE("connection_store_clear_disconnects_all", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  auto store = connection_store{};
  store.store(scoped_connection_type{sig.connect([&count] { ++count; })});
  store.store(scoped_connection_type{sig.connect([&count] { ++count; })});

  sig();
  CHECK(count == 2);

  store.clear();
  sig();
  CHECK(count == 2);
}

TEST_CASE("connection_store_takes_connections_after_clear", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  auto store = connection_store{};
  store.store(scoped_connection_type{sig.connect([&count] { ++count; })});

  sig();
  CHECK(count == 1);

  store.clear();
  sig();
  CHECK(count == 1);

  store.store(scoped_connection_type{sig.connect([&count] { ++count; })});
  sig();
  CHECK(count == 2);
}

} // namespace arcstd::signal
