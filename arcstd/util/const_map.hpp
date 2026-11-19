#pragma once

#include "arcstd/meta/type_traits.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/ranges.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

namespace arcstd::util
{
namespace detail
{

template<typename T>
struct is_pair : std::false_type
{};
template<typename FirstType, typename SecondType>
struct is_pair<std::pair<FirstType, SecondType>> : std::true_type
{};
template<typename T>
constexpr bool is_pair_v = is_pair<T>::value;

template<typename FirstType, typename SecondType, bool BidirectionalFlag>
concept mappable_types = requires {
  requires !BidirectionalFlag || !std::is_same_v<FirstType, SecondType>;
  requires std::equality_comparable<FirstType>;
  requires !BidirectionalFlag || std::equality_comparable<SecondType>;
};

///
/// A bimap whose two types compare with each other must be asked with the exact type \p T, else it
/// would not know which side to look up.
///
template<typename T, typename F, typename S, bool BidirectionalFlag>
concept explicit_if_equality_comparable =
  !BidirectionalFlag ||
  std::is_same_v<T, std::conditional_t<std::equality_comparable_with<F, S>, F, T>>;

template<typename T, typename F, typename S, bool BidirectionalFlag>
concept comparable_with = explicit_if_equality_comparable<T, F, S, BidirectionalFlag> &&
                          std::equality_comparable_with<T, F>;

} // namespace detail

///
/// Map of \p N pairs looked up at compile time, by the first and, as bimap, by the second value.
/// \see make_const_map and make_const_bimap
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag = true>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
class const_map final
{
  // Typedefs
  // Strings compare as std::string_view: a constant expression may not compare pointers
  template<typename C>
  using comparable_type =
    std::conditional_t<std::is_convertible_v<C, std::string_view>, std::string_view, C>;

  // Variables
  const std::array<std::pair<FirstType, SecondType>, N> map_;

public: // Typedefs
  using value_type = std::remove_const_t<decltype(map_)>::value_type;
  using size_type = std::size_t;
  using first_type = value_type::first_type;
  using second_type = value_type::second_type;
  using const_iterator = typename std::array<value_type, N>::const_iterator;
  using const_reverse_iterator = typename std::array<value_type, N>::const_reverse_iterator;

public: // Structors
  template<typename... P>
  // NOLINTNEXTLINE(misc-explicit-constructor) a map is written as a braced list of its pairs
  constexpr const_map(P&&... p)
    requires(meta::are_same_v<std::pair<first_type, second_type>, std::remove_cvref_t<P>...>);

public: // Accessors
  ///
  /// \return true if a pair has the first value \p first, \see
  /// detail::explicit_if_equality_comparable
  ///
  template<typename F>
  [[nodiscard]] constexpr auto contains(const F& first) const -> bool
    requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>);

  ///
  /// \return true if a pair has the second value \p second
  ///
  template<typename S>
  [[nodiscard]] constexpr auto contains(const S& second) const -> bool
    requires(
      detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag
    );

  ///
  /// \return the second value of \p first, \see detail::explicit_if_equality_comparable
  /// \throws util::exception if there is none
  ///
  template<typename F>
  [[nodiscard]] constexpr auto at(const F& first) const -> const second_type&
    requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>);

  ///
  /// \return the first value of \p second
  /// \throws util::exception if there is none
  ///
  template<typename S>
  [[nodiscard]] constexpr auto at(const S& second) const -> const first_type&
    requires(
      detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag
    );
  [[nodiscard]] constexpr auto size() const -> size_type;

public: // Iterators
  [[nodiscard]] constexpr auto begin() const -> const_iterator;
  [[nodiscard]] constexpr auto cbegin() const -> const_iterator;
  [[nodiscard]] constexpr auto end() const -> const_iterator;
  [[nodiscard]] constexpr auto cend() const -> const_iterator;
  [[nodiscard]] constexpr auto rbegin() const -> const_reverse_iterator;
  [[nodiscard]] constexpr auto crbegin() const -> const_reverse_iterator;
  [[nodiscard]] constexpr auto rend() const -> const_reverse_iterator;
  [[nodiscard]] constexpr auto crend() const -> const_reverse_iterator;

private: // Implementation
  ///
  /// \return true if \p lhs equals \p rhs, compared as comparable_type
  ///
  [[nodiscard]] constexpr auto is_equal(const auto& lhs, const auto& rhs) const -> bool;
};

template<typename T>
struct is_const_map : std::false_type
{};
template<typename FirstType, typename SecondType, std::size_t N>
struct is_const_map<const_map<FirstType, SecondType, N>> : std::true_type
{};
template<typename T>
constexpr bool is_const_map_v = is_const_map<T>::value;

template<typename T>
concept const_map_type = is_const_map_v<T>;

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename... P>
constexpr const_map<FirstType, SecondType, N, BidirectionalFlag>::const_map(P&&... p)
  requires(meta::are_same_v<std::pair<first_type, second_type>, std::remove_cvref_t<P>...>)
  : map_{std::array{std::forward<P>(p)...}}
{
  std::ranges::for_each(
    util::ranges::index_view_to(sizeof...(P)),
    [&](const auto i)
    {
      std::ranges::for_each(
        util::ranges::index_view_between(i + 1, sizeof...(P)),
        [&](const auto j)
        {
          if(is_equal(map_.at(i).first, map_.at(j).first))
          {
            throw util::exception("duplicates in first elements");
          }
          if constexpr(BidirectionalFlag)
          {
            if(is_equal(map_.at(i).second, map_.at(j).second))
            {
              throw util::exception("duplicates in second elements");
            }
          }
        }
      );
    }
  );
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename F>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::contains(
  const F& first
) const -> bool
  requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>)
{
  return std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.first, first); }) !=
         std::cend(map_);
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename S>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::contains(
  const S& second
) const -> bool
  requires(
    detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag
  )
{
  return std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.second, second); }) !=
         std::cend(map_);
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename F>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::at(const F& first) const
  -> const second_type&
  requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>)
{
  const auto iter =
    std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.first, first); });
  if(iter == std::cend(map_))
  {
    throw util::exception("first out of range");
  }
  return iter->second;
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename S>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::at(const S& second) const
  -> const first_type&
  requires(
    detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag
  )
{
  const auto iter =
    std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.second, second); });
  if(iter == std::cend(map_))
  {
    throw util::exception("second out of range");
  }
  return iter->first;
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::size() const -> size_type
{
  return map_.size();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::begin() const
  -> const_iterator
{
  return map_.begin();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::cbegin() const
  -> const_iterator
{
  return map_.cbegin();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::end() const -> const_iterator
{
  return map_.end();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::cend() const
  -> const_iterator
{
  return map_.cend();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::rbegin() const
  -> const_reverse_iterator
{
  return map_.rbegin();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::crbegin() const
  -> const_reverse_iterator
{
  return map_.crbegin();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::rend() const
  -> const_reverse_iterator
{
  return map_.rend();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::crend() const
  -> const_reverse_iterator
{
  return map_.crend();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::is_equal(
  const auto& lhs, const auto& rhs
) const -> bool
{
  return static_cast<comparable_type<decltype(lhs)>>(lhs) ==
         static_cast<comparable_type<decltype(rhs)>>(rhs);
}

// NOLINTBEGIN(modernize-avoid-c-arrays) a braced list of pairs gives its size only to a C array
namespace detail
{

///
/// \return the const_map of \p p, use make_const_map or make_const_bimap
///
template<typename F, typename S, std::size_t N, bool BidirectionalFlag, std::size_t... Is>
[[nodiscard]] consteval auto make_const_map_impl(
  std::pair<F, S> (&&p)[N], [[maybe_unused]] std::index_sequence<Is...> /*sequence*/
) -> auto
{
  return const_map<F, S, N, BidirectionalFlag>(std::move(p[Is])...);
}

} // namespace detail

///
/// \return the const_map of \p p, e.g. make_const_map<int, std::string_view>({{0, "e_0"}, {1,
/// "e_1"}})
///
template<typename F, typename S, std::size_t N>
[[nodiscard]] consteval auto make_const_map(std::pair<F, S> (&&p)[N]) -> auto
{
  static constexpr auto unidirectional = false;
  return detail::make_const_map_impl<F, S, N, unidirectional>(
    std::move(p), std::make_index_sequence<N>{}
  );
}

///
/// \return the const_map of \p p, looked up in both directions
///
template<typename F, typename S, std::size_t N>
[[nodiscard]] consteval auto make_const_bimap(std::pair<F, S> (&&p)[N]) -> auto
{
  static constexpr auto bidirectional = true;
  return detail::make_const_map_impl<F, S, N, bidirectional>(
    std::move(p), std::make_index_sequence<N>{}
  );
}
// NOLINTEND(modernize-avoid-c-arrays)

} // namespace arcstd::util
