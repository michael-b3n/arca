#pragma once

#include <cstdint>
#include <format>

namespace arcstd::util
{
namespace detail
{

///
/// \return the next value of the counter all unique IDs share
///
[[nodiscard]] auto next_uid_value() -> std::uint64_t;

} // namespace detail

///
/// ID unique among all IDs of the program, of a type of its own per \p Tag.
///
template<typename Tag>
class uid final
{
  // Friends
  friend struct std::formatter<uid<Tag>>;

  // Variables
  std::uint64_t value_;

public: // Typedefs
  using tag_type = Tag;
  using underlying_type = decltype(value_);

public: // Static interface
  ///
  /// \return an ID no other one has
  ///
  [[nodiscard]] static auto new_uid() -> uid<tag_type>;

public: // Structors
  uid();

public: // Operators
  constexpr auto operator<=>(const uid&) const = default;

private: // Structors
  constexpr explicit uid(underlying_type value);
};

///
///
template<typename Tag>
auto uid<Tag>::new_uid() -> uid<tag_type>
{
  return uid{detail::next_uid_value()};
}

///
///
template<typename Tag>
uid<Tag>::uid()
  : uid{new_uid()}
{
}

///
///
template<typename Tag>
constexpr uid<Tag>::uid(const underlying_type value)
  : value_{value}
{
}

} // namespace arcstd::util

///
///
template<typename T>
struct std::formatter<arcstd::util::uid<T>> : std::formatter<std::uint64_t>
{
  auto format(const arcstd::util::uid<T> id, std::format_context& ctx) const
  {
    return formatter<std::uint64_t>::format(id.value_, ctx);
  }
};
