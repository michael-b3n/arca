#pragma once

#include <boost/signals2.hpp>

#include <functional>

namespace arcstd::signal
{

///
/// Signal of the signature \p T, calling it calls the connected slots.
///
template<typename T>
using signal_type = boost::signals2::signal<T>;

template<typename T>
struct is_signal_type final : std::false_type
{};
template<typename T>
struct is_signal_type<signal_type<T>> final : std::true_type
{};

template<typename T>
inline constexpr auto is_signal_type_v = is_signal_type<std::remove_cvref_t<T>>::value;

template<typename T>
concept signal_like = is_signal_type_v<T>;

namespace detail
{

template<typename T>
struct signal_signature_extractor;

template<typename R, typename... Args>
struct signal_signature_extractor<R(Args...)> final
{
  using return_type = R;
  using args_tuple = std::tuple<Args...>;
};

} // namespace detail

///
/// The return type and the argument types of the signal \p T, of a slot and of an extended slot.
///
template<signal_like T>
struct signal_signature
{
private:
  using signature = detail::signal_signature_extractor<typename T::slot_type::signature_type>;
  using signature_extended =
    detail::signal_signature_extractor<typename T::extended_slot_type::signature_type>;
  static_assert(
    std::is_same_v<typename signature::return_type, typename signature_extended::return_type>
  );

public:
  using return_type = typename signature::return_type;
  using args_tuple = typename signature::args_tuple;
  using extended_args_tuple = typename signature_extended::args_tuple;
};

///
/// The slot and the extended slot of the signal \p T as \p F, std::function or
/// std::move_only_function.
///
template<signal_like T, template<typename...> typename F>
struct to_functional;

template<typename T>
struct to_functional<T, std::function> final
{
  using type = std::function<typename T::slot_type::signature_type>;
  using type_extended = std::function<typename T::extended_slot_type::signature_type>;
};
template<typename T>
struct to_functional<T, std::move_only_function> final
{
  using type = std::move_only_function<typename T::slot_type::signature_type>;
  using type_extended = std::move_only_function<typename T::extended_slot_type::signature_type>;
};

using connection_type = boost::signals2::connection;

///
/// Disconnects its slot when destroyed.
///
using scoped_connection_type = boost::signals2::scoped_connection;

///
/// Blocks a slot while it lives.
///
using connection_block_type = boost::signals2::shared_connection_block;

} // namespace arcstd::signal
