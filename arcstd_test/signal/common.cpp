#include <arcstd/signal/common.hpp>

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace arcstd::signal
{

TEST_CASE("is_signal_type_detects_signals", "[signal]")
{
  static_assert(is_signal_type_v<signal_type<void()>>);
  static_assert(is_signal_type_v<signal_type<int(float, double)>>);
  static_assert(!is_signal_type_v<int>);
  static_assert(!is_signal_type_v<std::function<void()>>);
  static_assert(!is_signal_type_v<void>);
}

TEST_CASE("signal_signature_names_return_and_argument_types", "[signal]")
{
  using sig1 = signal_type<void(int, float)>;
  using signature1 = signal_signature<sig1>;
  static_assert(std::is_void_v<typename signature1::return_type>);
  static_assert(std::is_same_v<typename signature1::args_tuple, std::tuple<int, float>>);

  using sig2 = signal_type<int(double)>;
  using signature2 = signal_signature<sig2>;
  static_assert(std::is_same_v<typename signature2::return_type, int>);
  static_assert(std::is_same_v<typename signature2::args_tuple, std::tuple<double>>);

  using sig3 = signal_type<void()>;
  using signature3 = signal_signature<sig3>;
  static_assert(std::is_void_v<typename signature3::return_type>);
  static_assert(std::is_same_v<typename signature3::args_tuple, std::tuple<>>);

  using sig4 = signal_type<std::string && (const std::vector<int>&)>;
  using signature4 = signal_signature<sig4>;
  static_assert(std::is_same_v<typename signature4::return_type, std::string&&>);
  static_assert(
    std::is_same_v<typename signature4::args_tuple, std::tuple<const std::vector<int>&>>
  );
}

TEST_CASE("to_functional_names_the_function_of_a_signal", "[signal]")
{
  using sig1 = signal_type<void(int, float)>;
  using func1 = to_functional<sig1, std::function>::type;
  using func1_extended = to_functional<sig1, std::function>::type_extended;
  static_assert(std::is_same_v<func1, std::function<void(int, float)>>);
  static_assert(
    std::is_same_v<func1_extended, std::function<void(const connection_type&, int, float)>>
  );

  using sig2 = signal_type<int(double)>;
  using func2 = to_functional<sig2, std::move_only_function>::type;
  using func2_extended = to_functional<sig2, std::move_only_function>::type_extended;
  static_assert(std::is_same_v<func2, std::move_only_function<int(double)>>);
  static_assert(
    std::is_same_v<func2_extended, std::move_only_function<int(const connection_type&, double)>>
  );
}

TEST_CASE("signal_type_connects_and_emits", "[signal]")
{
  auto sig = signal_type<void(int)>{};
  auto received = 0;

  const auto con = sig.connect([&received](int value) { received = value; });

  sig(42);
  CHECK(received == 42);

  sig(99);
  CHECK(received == 99);
}

TEST_CASE("signal_type_calls_every_slot", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  const auto con1 = sig.connect([&count] { ++count; });
  const auto con2 = sig.connect([&count] { ++count; });

  sig();
  CHECK(count == 2);

  sig();
  CHECK(count == 4);
}

TEST_CASE("scoped_connection_disconnects_on_destruction", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  {
    const auto scoped = scoped_connection_type{sig.connect([&count] { ++count; })};
    sig();
    CHECK(count == 1);
  }

  sig();
  CHECK(count == 1);
}

TEST_CASE("connection_type_disconnects_by_hand", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  const auto con = sig.connect([&count] { ++count; });
  sig();
  CHECK(count == 1);

  con.disconnect();
  sig();
  CHECK(count == 1);
}

TEST_CASE("connection_block_type_blocks_the_signal_while_it_lives", "[signal]")
{
  auto sig = signal_type<void()>{};
  auto count = 0;

  const auto con = sig.connect([&count] { ++count; });

  sig();
  CHECK(count == 1);

  {
    const auto blocker = connection_block_type{con};
    sig();
    CHECK(count == 1);
  }

  sig();
  CHECK(count == 2);
}

} // namespace arcstd::signal
