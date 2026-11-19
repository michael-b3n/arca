#include <arcstd/util/find_uint.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>

namespace arcstd::util
{

TEST_CASE("find_uint_reads_the_number_at_the_start", "[util]")
{
  using result = find_uint_t::result;
  static_assert(find_uint("42") == result{.value = 42, .post_value_offset = 2});
  CHECK(find_uint("123abc") == result{.value = 123, .post_value_offset = 3});
  CHECK(find_uint("007 ") == result{.value = 7, .post_value_offset = 3});
  CHECK(find_uint("4294967295") == result{.value = 4294967295, .post_value_offset = 10});
}

TEST_CASE("find_uint_finds_none_without_a_number_at_the_start", "[util]")
{
  static_assert(find_uint("") == std::nullopt);
  CHECK(find_uint("abc1") == std::nullopt);
  CHECK(find_uint(" 1") == std::nullopt);
  CHECK(find_uint("-1") == std::nullopt);
  // Too large for 32 bits
  CHECK(find_uint("4294967296") == std::nullopt);
}

} // namespace arcstd::util
