#pragma once

#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

namespace arcstd::util
{

///
/// Formats values with std::format.
///
struct format final
{
  ///
  /// \return the elements of \p range formatted, \p delimiter between them
  ///
  [[nodiscard]] static auto join(const std::ranges::range auto& range, std::string_view delimiter)
    -> std::string;

  ///
  /// \return \p value formatted, "nullopt" if it holds none
  ///
  template<typename T>
  [[nodiscard]] static auto to_string(const std::optional<T>& value) -> std::string;
};

///
///
auto format::join(const std::ranges::range auto& range, const std::string_view delimiter)
  -> std::string
{
  return range | std::views::transform([](const auto& item) { return std::format("{}", item); }) |
         std::views::join_with(delimiter) | std::ranges::to<std::string>();
}

///
///
template<typename T>
auto format::to_string(const std::optional<T>& value) -> std::string
{
  return value ? std::format("{}", *value) : "nullopt";
}

} // namespace arcstd::util
