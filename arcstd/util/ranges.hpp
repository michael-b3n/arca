#pragma once

#include <concepts>
#include <ranges>
#include <type_traits>

namespace arcstd::util::ranges
{

///
/// \return the indices of \p range
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view(const R& range) -> auto
{
  return std::views::iota(std::ranges::range_size_t<R>{0}, std::ranges::size(range));
}

///
/// \return the indices of \p range from \p begin, none if it lies beyond
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view_from(
  const R& range, const std::ranges::range_size_t<R> begin
) -> auto
{
  const auto size = std::ranges::size(range);
  return std::views::iota(std::min(begin, size), size);
}

///
/// \return the indices of \p range up to \p end, all of them if it lies beyond
///
template<std::ranges::sized_range R>
[[nodiscard]] constexpr auto index_view_to(const R& range, const std::ranges::range_size_t<R> end)
  -> auto
{
  const auto size = std::ranges::size(range);
  return std::views::iota(std::ranges::range_size_t<R>{0}, std::min(end, size));
}

///
/// \return the indices 0 up to \p end, none if it is negative
///
[[nodiscard]] constexpr auto index_view_to(const std::integral auto end) -> auto
{
  return std::views::iota(decltype(end){0}, std::max(decltype(end){0}, end));
}

///
/// \return the indices from \p first up to \p second, in their common type
///
[[nodiscard]] constexpr auto index_view_between(
  const std::integral auto first, const std::integral auto second
) -> auto
{
  using common_type =
    std::common_type_t<std::decay_t<decltype(first)>, std::decay_t<decltype(second)>>;
  return std::views::iota(
    std::min(static_cast<common_type>(first), static_cast<common_type>(second)),
    std::max(static_cast<common_type>(first), static_cast<common_type>(second))
  );
}

} // namespace arcstd::util::ranges
