#pragma once

#include <memory>
#include <utility>

namespace arcstd::util
{

///
/// Holds a T that is never destroyed, for a static that must outlive the statics destroyed at the end of the
/// process. It needs no heap, unlike a static pointer made with new.
///
template<typename T>
class no_destructor final
{
  // Variables
  union
  {
    T value;
  };

public: // Structors
  ///
  /// Construct the held T of \p args.
  ///
  template<typename... Args>
  explicit no_destructor(Args&&... args);
  ~no_destructor() noexcept;

  no_destructor(const no_destructor&) = delete;
  no_destructor(no_destructor&&) = delete;
  auto operator=(const no_destructor&) -> no_destructor& = delete;
  auto operator=(no_destructor&&) -> no_destructor& = delete;

public: // Accessors
  [[nodiscard]] auto operator*() -> T&;
  [[nodiscard]] auto operator->() -> T*;
};

///
///
template<typename T>
template<typename... Args>
no_destructor<T>::no_destructor(Args&&... args)
  : value{std::forward<Args>(args)...}
{
}

///
/// Leaves the held T as it is, that is the purpose.
///
template<typename T>
no_destructor<T>::~no_destructor() noexcept // NOLINT(modernize-use-equals-default) a default one would destroy it
{
}

///
///
template<typename T>
auto no_destructor<T>::operator*() -> T&
{
  return value; // NOLINT(cppcoreguidelines-pro-type-union-access) the one member, constructed
}

///
///
template<typename T>
auto no_destructor<T>::operator->() -> T*
{
  return std::addressof(value); // NOLINT(cppcoreguidelines-pro-type-union-access) the one member, constructed
}

} // namespace arcstd::util
