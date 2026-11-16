#pragma once

#include "arcstd/setting/property_node.hpp"
#include "arcstd/setting/setting_common.hpp"

#include <mutex>
#include <utility>
#include <variant>

namespace arcstd::setting
{

///
/// Thread safe property, the value of a node of a settings file. It reads the value from the node once and writes
/// each change back, \see property_node.
///
template<underlying_setting_type T>
class property final
{
  // Typedefs
  using erased_type = setting_type_erased_type_from<T>;

  // Variables
  mutable std::mutex mtx_;
  property_node node_;
  T value_;

public: // Typedefs
  using value_type = T;

public: // Structors
  ///
  /// Property of \p node with its value, \p default_value if the node holds none, or one of another type or naming
  /// no value of the enum. The node holds the value from now on.
  ///
  property(property_node node, value_type default_value);
  property(const property<value_type>& other);
  property(property<value_type>&& other) noexcept;
  ~property() noexcept = default;

public: // Operators
  auto operator=(const property<value_type>& other) & -> property<value_type>&;
  auto operator=(property<value_type>&& other) & noexcept -> property<value_type>&;
  auto operator==(const property<value_type>& other) const -> bool;

public: // Accessors
  ///
  /// Get property value.
  /// \return value of property
  ///
  [[nodiscard]] auto value() const -> value_type;

public: // Modifiers
  ///
  /// Set property value, a change is written to the node.
  ///
  auto value(const value_type& value) -> void;

  ///
  /// Exchange property value, a change is written to the node.
  /// \return old property value
  ///
  [[nodiscard]] auto exchange(const value_type& new_value) -> value_type;

private: // Structors
  property(const property<value_type>& other, const std::scoped_lock<std::mutex>& lock);
  property(property<value_type>&& other, const std::scoped_lock<std::mutex>& lock) noexcept;

private: // Implementation
  auto write() -> void;
};

///
///
template<underlying_setting_type T>
property<T>::property(property_node node, value_type default_value)
  : node_{std::move(node)}
  , value_{node_.read(setting_type_erased_variant{std::in_place_type<erased_type>})
             .and_then([](const auto& stored) { return try_from_type_erased_setting<T>(std::get<erased_type>(stored)); })
             .value_or(std::move(default_value))}
{
  write();
}

///
///
template<underlying_setting_type T>
property<T>::property(const property<value_type>& other)
  : property{other, std::scoped_lock{other.mtx_}}
{
}

///
///
template<underlying_setting_type T>
property<T>::property(property<value_type>&& other) noexcept
  : property{std::move(other), std::scoped_lock{other.mtx_}}
{
}

///
///
template<underlying_setting_type T>
auto property<T>::operator=(const property<value_type>& other) & -> property<value_type>&
{
  if(this != &other)
  {
    const auto lock = std::scoped_lock{mtx_, other.mtx_};
    node_ = other.node_;
    value_ = other.value_;
  }
  return *this;
}

///
///
template<underlying_setting_type T>
auto property<T>::operator=(property<value_type>&& other) & noexcept -> property<value_type>&
{
  if(this != &other)
  {
    const auto lock = std::scoped_lock{mtx_, other.mtx_};
    node_ = std::move(other.node_);
    value_ = std::move(other.value_);
  }
  return *this;
}

///
///
template<underlying_setting_type T>
auto property<T>::operator==(const property<value_type>& other) const -> bool
{
  const auto lock = std::scoped_lock{mtx_, other.mtx_};
  return value_ == other.value_;
}

///
///
template<underlying_setting_type T>
auto property<T>::value() const -> value_type
{
  const auto lock = std::scoped_lock{mtx_};
  return value_;
}

///
///
template<underlying_setting_type T>
auto property<T>::value(const value_type& value) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  if(value_ != value)
  {
    value_ = value;
    write();
  }
}

///
///
template<underlying_setting_type T>
auto property<T>::exchange(const value_type& new_value) -> value_type
{
  const auto lock = std::scoped_lock{mtx_};
  const auto old_value = value_;
  if(value_ != new_value)
  {
    value_ = new_value;
    write();
  }
  return old_value;
}

///
///
template<underlying_setting_type T>
property<T>::property(const property<value_type>& other, [[maybe_unused]] const std::scoped_lock<std::mutex>& lock)
  : node_{other.node_}
  , value_{other.value_}
{
}

///
///
template<underlying_setting_type T>
property<T>::property(property<value_type>&& other, [[maybe_unused]] const std::scoped_lock<std::mutex>& lock) noexcept
  : node_{std::move(other.node_)}
  , value_{std::move(other.value_)}
{
}

///
///
template<underlying_setting_type T>
auto property<T>::write() -> void
{
  // The alternative is named, else e.g. a bool would be taken for a std::optional<bool>
  node_.write(
    setting_type_erased_variant{std::in_place_type<erased_type>, create_setting_value_converter<T, erased_type>()(value_)}
  );
}

} // namespace arcstd::setting
