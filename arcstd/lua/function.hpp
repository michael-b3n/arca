#pragma once

#include "arcstd/lua/value.hpp"
#include "arcstd/lua/value_conversion.hpp"
#include "arcstd/util/exception.hpp"

#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace arcstd::lua
{

///
/// Function for scripts: it takes the arguments of a call and returns its results, in order.
/// An exception it throws is an error for the script. \see to_function
///
using function = std::function<std::vector<value>(std::span<const value>)>;

///
/// Functions of a table for scripts, by name.
///
using function_table = std::map<std::string, function>;

namespace detail
{

///
/// Result of a function that may fail: a script gets the value, or nil and the error, like io.open does it.
///
template<typename T>
struct is_expected_result final : std::false_type
{};
template<value_convertible T, value_convertible E>
struct is_expected_result<std::expected<T, E>> final : std::true_type
{};
template<typename T>
concept expected_result = is_expected_result<T>::value;

///
/// What a function for scripts returns: nothing, a value or an expected_result.
///
template<typename T>
concept function_result = std::is_void_v<T> || value_convertible<T> || expected_result<T>;

///
/// \return the argument \p index of a call from a script, nil if it is left out
/// \throw util::exception if it is no T, the script gets it as error
///
template<value_convertible T>
[[nodiscard]] auto argument(const std::span<const value> arguments, const std::size_t index) -> T
{
  const auto left_out = value{};
  auto converted = from_value<T>(index < arguments.size() ? arguments[index] : left_out);
  if(!converted)
  {
    throw util::exception{std::format("argument {} is of another type", index + 1)};
  }
  return std::move(*converted);
}

///
/// What a function for scripts takes: a value_convertible type, by value or const reference.
///
template<typename T>
concept function_parameter = value_convertible<std::remove_cvref_t<T>> && std::convertible_to<std::remove_cvref_t<T>, T>;

///
/// \return the function a script calls in place of \p callable, \see to_function
///
template<typename R, typename... A>
  requires(function_result<R> && (function_parameter<A> && ...))
[[nodiscard]] auto wrapped(std::function<R(A...)> callable) -> function
{
  return [callable = std::move(callable)](const std::span<const value> arguments) -> std::vector<value>
  {
    const auto call = [&]<std::size_t... I>(std::index_sequence<I...>) -> R
    { return callable(argument<std::remove_cvref_t<A>>(arguments, I)...); };
    // Each kind of function_result
    if constexpr(std::is_void_v<R>)
    {
      call(std::index_sequence_for<A...>{});
      return {};
    }
    else if constexpr(value_convertible<R>)
    {
      return {to_value(call(std::index_sequence_for<A...>{}))};
    }
    else if constexpr(expected_result<R>)
    {
      if(const auto result = call(std::index_sequence_for<A...>{}))
      {
        return {to_value(*result)};
      }
      else
      {
        return {value{}, to_value(result.error())};
      }
    }
    else
    {
      static_assert(false, "function_result conversion unsupported");
    }
  };
}

} // namespace detail

///
/// Callables to_function converts: one call signature, e.g. a lambda that is not generic, its parameters and its
/// result value_convertible.
///
template<typename F>
concept function_convertible = requires(F&& callable) { detail::wrapped(std::function{std::forward<F>(callable)}); };

///
/// \return \p callable as function for scripts. It takes and returns C++ values, \see value_conversion. An argument
/// of another type is an error for the script, one left out is nil, extra ones are ignored.
///
template<function_convertible F>
[[nodiscard]] auto to_function(F&& callable) -> function
{
  return detail::wrapped(std::function{std::forward<F>(callable)});
}

} // namespace arcstd::lua
