#pragma once

#include "arcstd/export.hpp"
#include "arcstd/meta/for_each.hpp"
#include "arcstd/meta/type_traits.hpp"
#include "arcstd/setting/property.hpp"
#include "arcstd/setting/setting.hpp"
#include "arcstd/setting/setting_common.hpp"
#include "arcstd/setting/setting_type_erased.hpp"
#include "arcstd/setting/setting_validator.hpp"
#include "arcstd/signal/adapter.hpp"
#include "arcstd/util/contains.hpp"
#include "arcstd/util/enum.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/util/non_owning_ptr.hpp"

#include <filesystem>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace arcstd::setting
{

class property_tree;

namespace detail
{

///
/// Conditionally creates the default validator of a new setting.
/// For enum values, optional or list enums, a list validator is created
/// \return default validator types
///
template<underlying_setting_type T>
[[nodiscard]] auto conditional_default_validator() -> setting_validator<T>
{
  // Unpacks a type from its wrapper. std::vector<T> and std::optional<T>
  // will return T, which is checked for enum type.
  if constexpr(std::is_enum_v<meta::remove_wrapper_t<T>>)
  {
    using enum_type = meta::remove_wrapper_t<T>;
    return std::make_shared<setting_validator_list<T>>(util::enum_values<enum_type>());
  }
  else
  {
    return std::make_shared<setting_validator_unbound>();
  }
}

} // namespace detail

///
/// Signals emitted by the setting store.
///
struct setting_store_signals final
{
  signal::signal_type<void(std::string)> setting_created;
};

///
/// Setting store. This class owns the setting objects and keeps their values in the settings file,
/// so it is required to live as long as the settings are used.
///
class ARCSTD_EXPORT setting_store final : public signal::adapter<setting_store_signals>
{
  // Typedefs
  template<underlying_setting_type_erased_type T>
  using setting_type_erased_uptr_type = std::unique_ptr<setting_type_erased<T>>;
  using setting_type_erased_uptr_variant_type = meta::for_each_t<setting_type_erased_variant, setting_type_erased_uptr_type>;

  struct setting_uptr_data final
  {
    std::string path;
    setting_type_erased_uptr_variant_type setting;
  };

  // Variables
  const std::filesystem::path data_folder_;
  const std::shared_ptr<property_tree> tree_;
  mutable std::mutex mtx_;
  std::vector<setting_uptr_data> settings_;

public: // Typedefs
  using setting_signals_type = setting_signals;
  using setting_type_erased_variant_type = setting_type_erased_variant;

  template<underlying_setting_type T>
  using setting_non_owning_ptr_type = util::non_owning_ptr<setting<T>>;
  template<underlying_setting_type_erased_type T>
  using setting_type_erased_non_owning_ptr_type = util::non_owning_ptr<setting_type_erased<T>>;
  using setting_type_erased_non_owning_ptr_variant_type =
    meta::for_each_t<setting_type_erased_variant, setting_type_erased_non_owning_ptr_type>;

  ///
  /// Settings data struct.
  ///
  struct setting_data final
  {
    std::string path;
    setting_type_erased_non_owning_ptr_variant_type setting;
  };

  template<underlying_setting_type T>
  using validator_type = setting_validator<T>;
  using validator_unbound_type = setting_validator_unbound;
  template<underlying_setting_type T>
  using validator_range_type = setting_validator_range<T>;
  template<underlying_setting_type T>
  using validator_list_type = setting_validator_list<T>;
  template<underlying_setting_type_erased_type T>
  using validator_range_type_erased_type = setting_validator_range_type_erased<T>;
  template<underlying_setting_type_erased_type T>
  using validator_list_type_erased_type = setting_validator_list_type_erased<T>;

public: // Constants
  static constexpr std::string_view settings_file_name = "settings.xml";
  static constexpr std::string_view settings_root_name = "settings";

public: // Static interface
  ///
  /// \return the path of the settings file in \p data_folder
  ///
  [[nodiscard]] static auto settings_file_path(const std::filesystem::path& data_folder) -> std::filesystem::path;

public: // Structors
  ///
  /// Construct the settings, stored in the settings file of \p data_folder, the folder of the data of the app.
  ///
  explicit setting_store(std::filesystem::path data_folder);

public: // Accessors
  ///
  /// \return the folder of the data of the app, the settings are stored in
  ///
  [[nodiscard]] auto data_folder() const -> const std::filesystem::path&;

  ///
  /// Access all created settings.
  /// \return list of all created settings
  ///
  [[nodiscard]] auto type_erased_settings() const -> std::vector<setting_data>;

  ///
  /// Access a type erased setting for the specified path.
  /// \return type erased setting or nullopt if no setting with the specified path exists
  ///
  [[nodiscard]] auto type_erased_setting(const std::string& path) const
    -> std::optional<setting_type_erased_non_owning_ptr_variant_type>;

public: // Modifiers
  ///
  /// Create an unbound setting owned by this store, of a type known at runtime only, e.g. declared by the
  /// frontend, or access the existing one of \p path.
  /// \throws util::exception if a setting of \p path exists with a different value type
  /// \return the created or existing setting
  ///
  template<underlying_setting_type_erased_type T>
  [[nodiscard]] auto type_erased_setting(
    const std::string& path,
    T default_value,
    std::string postfix = {},
    validator_type<T> validator = detail::conditional_default_validator<T>()
  ) -> setting_type_erased_non_owning_ptr_variant_type;

  ///
  /// Create or access a setting with no postfix, naming only the validator. \see type_erased_setting
  /// \return the newly created or the already existing setting
  ///
  template<underlying_setting_type_erased_type T>
  [[nodiscard]] auto type_erased_setting(const std::string& path, T default_value, validator_type<T> validator)
    -> setting_type_erased_non_owning_ptr_variant_type;

  ///
  /// Create a new setting. The setting will be owned by this store and has static lifetime.
  /// \p postfix is displayed behind the value, e.g. the unit it is measured in.
  /// \return non owning pointer to the newly created setting
  ///
  template<underlying_setting_type T>
  [[nodiscard]] auto create_setting(
    const std::string& path,
    T default_value,
    std::string postfix = {},
    validator_type<T> validator = detail::conditional_default_validator<T>()
  ) -> setting_non_owning_ptr_type<T>;

  ///
  /// Create a new setting with no postfix, naming only the validator. \see create_setting
  /// \return non owning pointer to the newly created setting
  ///
  template<underlying_setting_type T>
  [[nodiscard]] auto create_setting(const std::string& path, T default_value, validator_type<T> validator)
    -> setting_non_owning_ptr_type<T>;

private: // Implementation
  ///
  /// Create the setting of \p path with the value of the property tree, \p default_value if the tree has none.
  /// \return the entry owning the setting, and the setting
  ///
  template<underlying_setting_type T>
  [[nodiscard]] auto make_setting(const std::string& path, T default_value, std::string postfix, validator_type<T> validator)
    -> std::pair<setting_uptr_data, setting_non_owning_ptr_type<T>>;
};

///
///
template<underlying_setting_type_erased_type T>
auto setting_store::type_erased_setting(
  const std::string& path, T default_value, std::string postfix, validator_type<T> validator
) -> setting_type_erased_non_owning_ptr_variant_type
{
  auto lock = std::unique_lock{mtx_};
  auto it = std::ranges::find_if(settings_, [&path](const auto& data) { return data.path == path; });
  const auto setting_exists = it != std::ranges::cend(settings_);
  if(!setting_exists)
  {
    settings_.emplace_back(make_setting(path, std::move(default_value), std::move(postfix), std::move(validator)).first);
    it = std::prev(settings_.end());
  }
  const auto setting_ptr =
    std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, it->setting);
  lock.unlock();

  // If the setting already exists, check if the value type is the same as the requested type.
  if(setting_exists)
  {
    const auto is_same_value_type = std::visit(
      [](const auto setting)
      {
        using value_type = std::remove_pointer_t<std::remove_cvref_t<decltype(setting)>>::value_type;
        return std::is_same_v<value_type, std::remove_cvref_t<T>>;
      },
      setting_ptr
    );
    if(!is_same_value_type)
    {
      throw util::exception{std::format("setting exists with different value type: path=\"{}\"", path)};
    }
  }
  else
  {
    notify(&setting_store_signals::setting_created, path);
  }
  return setting_ptr;
}

///
///
template<underlying_setting_type_erased_type T>
auto setting_store::type_erased_setting(const std::string& path, T default_value, validator_type<T> validator)
  -> setting_type_erased_non_owning_ptr_variant_type
{
  return type_erased_setting<T>(path, std::move(default_value), {}, std::move(validator));
}

///
///
template<underlying_setting_type T>
auto setting_store::create_setting(const std::string& path, T default_value, std::string postfix, validator_type<T> validator)
  -> setting_non_owning_ptr_type<T>
{
  auto [entry, setting] = make_setting(path, std::move(default_value), std::move(postfix), std::move(validator));
  {
    const auto lock = std::scoped_lock{mtx_};
    const auto contains_path = util::contains(settings_, [&path](const auto& data) { return data.path == path; });
    if(contains_path)
    {
      throw util::exception(std::format("setting already created: path=\"{}\"", path));
    }
    settings_.emplace_back(std::move(entry));
  }
  notify(&setting_store_signals::setting_created, path);
  return setting;
}

///
///
template<underlying_setting_type T>
auto setting_store::create_setting(const std::string& path, T default_value, validator_type<T> validator)
  -> setting_non_owning_ptr_type<T>
{
  return create_setting<T>(path, std::move(default_value), {}, std::move(validator));
}

///
///
template<underlying_setting_type T>
auto setting_store::make_setting(const std::string& path, T default_value, std::string postfix, validator_type<T> validator)
  -> std::pair<setting_uptr_data, setting_non_owning_ptr_type<T>>
{
  using type_erased_type = setting_type_erased<setting_type_erased_type_from<T>>;
  const auto created = std::make_shared<setting<T>>(
    path,
    property<T>{
      property_node{tree_, std::format("{}.{}", settings_root_name, path)},
      std::move(default_value)
  },
    std::move(postfix),
    std::move(validator)
  );
  return {
    setting_uptr_data{.path = path, .setting = std::make_unique<type_erased_type>(created)},
    created.get()
  };
}

} // namespace arcstd::setting
