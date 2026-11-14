#include "arcstd/setting/property_node.hpp"
#include "arcstd/setting/property_parser.hpp"
#include "arcstd/setting/property_tree.hpp"
#include "arcstd/util/exception.hpp"

#include <mutex>
#include <utility>
#include <variant>

namespace arcstd::setting
{

///
///
property_node::property_node(std::shared_ptr<property_tree> tree, std::string path)
  : tree_{std::move(tree)}
  , path_{std::move(path)}
{
  if(path_.empty())
  {
    throw util::exception("property node without path");
  }
}

///
///
auto property_node::read(const setting_type_erased_variant& type) const -> std::optional<setting_type_erased_variant>
{
  const auto lock = std::scoped_lock{tree_->mtx_};
  return std::visit(
    [&]<typename T>(const T&)
    {
      // The alternative is named, else e.g. a bool would be taken for a std::optional<bool>
      return property_parser::read<T>(property_path_type{path_}, tree_->tree_)
        .transform([](T stored) { return setting_type_erased_variant{std::in_place_type<T>, std::move(stored)}; });
    },
    type
  );
}

///
///
auto property_node::write(const setting_type_erased_variant& value) -> void
{
  const auto lock = std::scoped_lock{tree_->mtx_};
  std::visit([&](const auto& v) { property_parser::write(property_path_type{path_}, tree_->tree_, v); }, value);
}

} // namespace arcstd::setting
