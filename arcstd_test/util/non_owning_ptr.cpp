#include <arcstd/util/non_owning_ptr.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace arcstd::util
{
namespace
{

template<typename T>
concept has_non_owning_ptr = requires { typename non_owning_ptr<T>; };

} // namespace

TEST_CASE("non_owning_ptr_is_a_plain_pointer", "[util]")
{
  static_assert(std::is_same_v<non_owning_ptr<int>, int*>);
  static_assert(std::is_same_v<non_owning_ptr<const int>, const int*>);
  // A pointer to a pointer is spelled out
  static_assert(!has_non_owning_ptr<int*>);

  auto value = 1;
  const non_owning_ptr<int> ptr = &value;
  *ptr = 2;
  CHECK(value == 2);
}

} // namespace arcstd::util
