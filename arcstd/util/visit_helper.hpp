#pragma once

#include <variant>

namespace arcstd::util
{
namespace detail
{

///
/// Overload set of the lambdas \p Ts.
///
template<class... Ts>
// NOLINTNEXTLINE(misc-multiple-inheritance) the overload set of its lambdas
struct overloaded : Ts...
{
  using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace detail

///
/// Visit \p var with the overload set of \p lambdas.
/// \return what std::visit returns
///
[[nodiscard]] constexpr auto visit_lambdas(auto&& var, auto&&... lambdas) -> decltype(auto)
{
  return std::visit(
    detail::overloaded{std::forward<decltype(lambdas)>(lambdas)...},
    std::forward<decltype(var)>(var)
  );
}

} // namespace arcstd::util
