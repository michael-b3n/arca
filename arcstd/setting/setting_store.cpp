#include "arcstd/setting/setting_store.hpp"
#include "arcstd/setting/property_tree.hpp"
#include "arcstd/system/filesystem.hpp"

#include <algorithm>
#include <ranges>

namespace arcstd::setting
{

///
///
auto setting_store::settings_file_path(const std::optional<std::string_view> folder_name) -> std::filesystem::path
{
  return system::filesystem::local_data_folder(folder_name) / settings_file_name;
}

///
///
setting_store::setting_store(const std::optional<std::string_view> folder_name)
  : data_folder_{system::filesystem::local_data_folder(folder_name)}
  , tree_{property_tree::create(data_folder_ / settings_file_name)}
{
}

///
///
auto setting_store::data_folder() const -> const std::filesystem::path&
{
  return data_folder_;
}

///
///
auto setting_store::type_erased_settings() const -> std::vector<setting_data>
{
  const auto lock = std::scoped_lock{mtx_};
  const auto to_setting_data = [](const auto& data)
  {
    return setting_data{
      data.path,
      std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, data.setting)
    };
  };
  return settings_ | std::views::transform(to_setting_data) | std::ranges::to<std::vector>();
}

///
///
auto setting_store::type_erased_setting(const std::string& path) const
  -> std::optional<setting_type_erased_non_owning_ptr_variant_type>
{
  const auto lock = std::scoped_lock{mtx_};
  const auto it = std::ranges::find_if(settings_, [&path](const auto& data) { return data.path == path; });
  if(it != std::ranges::cend(settings_))
  {
    return std::visit([](const auto& e) -> setting_type_erased_non_owning_ptr_variant_type { return e.get(); }, it->setting);
  }
  return std::nullopt;
}

} // namespace arcstd::setting
