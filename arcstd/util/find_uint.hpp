#pragma once

#include <cassert>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace arcstd::util
{

///
/// Reads the unsigned integer at the start of a text, \see find_uint.
///
struct find_uint_t final
{
  struct result final
  {
    std::uint32_t value;
    std::size_t post_value_offset; // of the first character after the number
    constexpr auto operator<=>(const result&) const = default;
  };

  ///
  /// \return the number \p text starts with, std::nullopt if it starts with none
  ///
  [[nodiscard]] constexpr auto operator()(std::string_view text) const -> std::optional<result>;
};

///
/// \see find_uint_t::operator()()
///
inline constexpr find_uint_t find_uint;

///
///
constexpr auto find_uint_t::operator()(const std::string_view text) const -> std::optional<result>
{
  std::uint32_t value{0};
  const auto retval = std::from_chars(text.data(), text.data() + text.size(), value);
  if(retval.ec != decltype(retval.ec){})
  {
    return std::nullopt;
  }
  const auto offset = std::ranges::distance(text.begin(), retval.ptr);
  assert(offset >= 0);
  return result{.value = value, .post_value_offset = static_cast<std::size_t>(offset)};
}

} // namespace arcstd::util
