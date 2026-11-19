#include <arcstd/lua/script_table.hpp>
#include <arcstd/lua/value.hpp>
#include <arcstd/lua/value_cast.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace arcstd::lua
{
namespace
{

using point = script_table<field<"x", std::int32_t>, field<"name", std::string>>;

///
/// \return \p v converted to a value and back, at compile time if called so
///
template<value_castable T>
[[nodiscard]] constexpr auto round_trip(const T& v) -> std::optional<T>
{
  return from_value<T>(to_value(v));
}

} // namespace

TEST_CASE("lua_value_converts_at_compile_time", "[lua]")
{
  STATIC_CHECK(round_trip(true) == true);
  STATIC_CHECK(round_trip(std::int32_t{-7}) == -7);
  STATIC_CHECK(round_trip(1.5) == 1.5);
  STATIC_CHECK(round_trip(std::string{"text"}) == "text");
  STATIC_CHECK(round_trip(std::chrono::seconds{3}) == std::chrono::seconds{3});
  STATIC_CHECK(round_trip(std::optional<std::int32_t>{5}).value() == 5);
  STATIC_CHECK(!round_trip(std::optional<std::int32_t>{}).value().has_value());
  STATIC_CHECK(
    round_trip(std::vector<std::int32_t>{1, 2, 3}) == std::vector<std::int32_t>{1, 2, 3}
  );
  STATIC_CHECK(round_trip(point{4, "p"}) == point{4, "p"});
}

TEST_CASE("lua_table_reads_its_sequence_at_compile_time", "[lua]")
{
  STATIC_CHECK(std::get<table>(to_value(std::vector<std::int32_t>{1, 2})).length() == 2);
  STATIC_CHECK(std::get<table>(to_value(std::vector<std::int32_t>{1, 2})).is_sequence());
  STATIC_CHECK(from_value<std::int32_t>(std::get<table>(to_value(point{4, "p"})).field("x")) == 4);
}

} // namespace arcstd::lua
