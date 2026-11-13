#include "arcstd/util/uid.hpp"

#include <atomic>

namespace arcstd::util::detail
{

///
///
auto next_uid_value() -> std::uint64_t
{
  static auto counter = std::atomic_uint64_t{0};
  return ++counter;
}

} // namespace arcstd::util::detail
