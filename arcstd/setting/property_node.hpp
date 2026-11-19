#pragma once

#include "arcstd/setting/setting_common.hpp"

#include <memory>
#include <optional>
#include <string>

namespace arcstd::setting
{

class property_tree;

///
/// Node of a settings file, where a property keeps its value, \see property.
///
class property_node final
{
  // Variables
  std::shared_ptr<property_tree> tree_;
  std::string path_;

public: // Structors
  ///
  /// Node \p path of \p tree, its sections separated by '.'.
  ///
  property_node(std::shared_ptr<property_tree> tree, std::string path);

public: // Accessors
  ///
  /// \return the value of the node, of the type \p type holds, std::nullopt if it holds none of
  /// that type
  ///
  [[nodiscard]] auto read(const setting_type_erased_variant& type) const
    -> std::optional<setting_type_erased_variant>;

public: // Modifiers
  ///
  /// Write \p value to the node.
  ///
  auto write(const setting_type_erased_variant& value) -> void;
};

} // namespace arcstd::setting
