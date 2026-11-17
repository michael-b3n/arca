#pragma once

#include "arcstd/meta/contains.hpp"
#include "arcstd/meta/for_each.hpp"
#include "arcstd/meta/remove_duplicates.hpp"
#include "arcstd/util/exception.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <variant>

namespace arcstd::util
{
namespace detail
{

template<typename P>
concept const_variant_mappable = requires {
  typename P::first_type;
  typename P::second_type;
};

} // namespace detail

///
/// Map of the pairs \p P, each of its own key and value type, the values are handed out as variant.
///
template<detail::const_variant_mappable... P>
class const_variant_map
{
  // Typedefs
  template<detail::const_variant_mappable E>
  using pairs_first_type = typename E::first_type;
  template<detail::const_variant_mappable E>
  using pairs_second_type = typename E::second_type;
  using first_types = meta::for_each_t<std::variant<P...>, pairs_first_type>;
  using second_types = meta::for_each_t<std::variant<P...>, pairs_second_type>;

  // Variables
  const std::tuple<P...> elements_;

public: // Typedefs
  using key_variant_type = meta::remove_duplicates_t<first_types>;
  using value_variant_type = meta::remove_duplicates_t<second_types>;
  using tuple_type = std::tuple<P...>;

public: // Constants
  static constexpr auto size = sizeof...(P);

public: // Structors
  // NOLINTNEXTLINE(misc-explicit-constructor) a map is written as a braced list of its pairs
  constexpr const_variant_map(P&&... p);

public: // Accessors
  ///
  /// \return true if the map has \p key
  ///
  template<typename T>
  [[nodiscard]] constexpr auto contains(const T& key) const -> bool
    requires(meta::contains_v<key_variant_type, std::decay_t<T>>);

  ///
  /// \return a copy of the value of \p key
  /// \throws util::exception if there is none
  ///
  template<typename T>
  [[nodiscard]] constexpr auto at(const T& key) const -> value_variant_type
    requires(meta::contains_v<key_variant_type, std::decay_t<T>>);

public: // Operations
  ///
  /// Call \p f with the value of \p key.
  /// \return true if the key was found
  ///
  template<typename T, typename F>
  constexpr auto visit(const T& key, F f) const -> bool
    requires(meta::contains_v<key_variant_type, std::decay_t<T>>);

  ///
  /// Call \p f with each key and value until it returns false.
  ///
  template<typename F>
  constexpr auto visit_until(F f) const -> void
    requires(std::is_invocable_r_v<bool, F, key_variant_type, value_variant_type>);
};

///
///
template<detail::const_variant_mappable... P>
constexpr const_variant_map<P...>::const_variant_map(P&&... p)
  : elements_{std::tuple{std::forward<P>(p)...}}
{
  auto get_key = [&]<std::size_t I> -> key_variant_type { return std::get<I>(elements_).first; };
  auto keys = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return std::array{get_key.template operator()<I>()...}; }(std::make_index_sequence<size>{});
  std::ranges::for_each(
    keys,
    [&](const auto& p)
    {
      if(std::ranges::count_if(keys, [&](const auto e) { return e == p; }) != 1)
      {
        throw exception{"duplicated keys not allowed"};
      }
    }
  );
}

///
///
template<detail::const_variant_mappable... P>
template<typename T>
constexpr auto const_variant_map<P...>::contains(const T& key) const -> bool
  requires(meta::contains_v<key_variant_type, std::decay_t<T>>)
{
  auto key_found = [&]<std::size_t I> -> bool
  { return key_variant_type(std::get<I>(elements_).first) == key_variant_type(key); };
  auto found = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return std::array{key_found.template operator()<I>()...}; }(std::make_index_sequence<size>{});
  return std::ranges::count(found, true) == 1;
}

///
///
template<detail::const_variant_mappable... P>
template<typename T>
constexpr auto const_variant_map<P...>::at(const T& key) const -> value_variant_type
  requires(meta::contains_v<key_variant_type, std::decay_t<T>>)
{
  auto do_on_key_found = [&]<std::size_t I> -> std::optional<value_variant_type>
  {
    if constexpr(
      std::is_same_v<typename std::tuple_element_t<I, tuple_type>::first_type, std::decay_t<T>>
    )
    {
      if(key_variant_type(std::get<I>(elements_).first) == key_variant_type(key))
      {
        return value_variant_type(std::get<I>(elements_).second);
      }
    }
    return std::nullopt;
  };
  return [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    const auto elements = std::array{do_on_key_found.template operator()<I>()...};
    const auto iter = std::ranges::find_if(elements, [](const auto& e) { return e.has_value(); });
    if(iter == std::cend(elements))
    {
      throw exception{"invalid key"};
    }
    return iter->value();
  }(std::make_index_sequence<size>{});
}

///
///
template<detail::const_variant_mappable... P>
template<typename T, typename F>
constexpr auto const_variant_map<P...>::visit(const T& key, F f) const -> bool
  requires(meta::contains_v<key_variant_type, std::decay_t<T>>)
{
  auto do_on_key_found = [&]<std::size_t I> -> bool
  {
    if constexpr(
      std::is_same_v<typename std::tuple_element_t<I, tuple_type>::first_type, std::decay_t<T>>
    )
    {
      if(key_variant_type(std::get<I>(elements_).first) == key_variant_type(key))
      {
        f(std::get<I>(elements_).second);
        return true;
      }
    }
    return false;
  };
  return [&]<std::size_t... I>(std::index_sequence<I...>)
  { return (... || do_on_key_found.template operator()<I>()); }(std::make_index_sequence<size>{});
}

///
///
template<detail::const_variant_mappable... P>
template<typename F>
constexpr auto const_variant_map<P...>::visit_until(F f) const -> void
  requires(std::is_invocable_r_v<bool, F, key_variant_type, value_variant_type>)
{
  auto visitor = [&]<std::size_t I>
  {
    return f(
      key_variant_type(std::get<I>(elements_).first),
      value_variant_type(std::get<I>(elements_).second)
    );
  };
  [&]<std::size_t... I>(std::index_sequence<I...>)
  { (... && visitor.template operator()<I>()); }(std::make_index_sequence<size>{});
}

} // namespace arcstd::util
