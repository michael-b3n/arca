#pragma once

#include <algorithm>
#include <concepts>
#include <type_traits>

namespace arcstd::util
{

///
/// \return true if \p container holds \p element
///
[[nodiscard]] constexpr auto contains(const std::ranges::range auto& container, const auto& element)
  -> bool
  requires(std::equality_comparable_with<
           std::ranges::range_value_t<decltype(container)>,
           decltype(element)>)
{
  return std::ranges::find(container, element) != std::ranges::cend(container);
}

///
/// \return true if an element of \p container fulfils \p pred
///
[[nodiscard]] constexpr auto contains(const std::ranges::range auto& container, const auto& pred)
  -> bool
  requires(
    !std::ranges::view<std::remove_cvref_t<decltype(container)>> &&
    std::predicate<decltype(pred), std::ranges::range_value_t<decltype(container)>>
  )
{
  return std::ranges::find_if(container, pred) != std::ranges::cend(container);
}

///
/// \return true if an element of \p view fulfils \p pred
///
[[nodiscard]] constexpr auto contains(std::ranges::view auto view, const auto& pred) -> bool
  requires(std::predicate<decltype(pred), std::ranges::range_value_t<decltype(view)>>)
{
  return std::ranges::find_if(view, pred) != std::ranges::cend(view);
}

} // namespace arcstd::util
