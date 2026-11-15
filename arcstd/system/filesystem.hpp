#pragma once

#include "arcstd/export.hpp"

#include <filesystem>
#include <optional>
#include <string_view>

namespace arcstd::system
{

///
/// Places of the file system the app uses.
///
struct ARCSTD_EXPORT filesystem final
{
  ///
  /// Get path to executable.
  /// \return path to executable location
  ///
  [[nodiscard]] static auto executable_location() -> std::filesystem::path;

  ///
  /// Get path to executable folder.
  /// \return path to executable folder
  ///
  [[nodiscard]] static auto executable_folder() -> std::filesystem::path;

  ///
  /// Get path to the local data folder \p folder_name, named after the executable if not set.
  /// \return path to local data
  ///
  [[nodiscard]] static auto local_data_folder(std::optional<std::string_view> folder_name = std::nullopt)
    -> std::filesystem::path;
};

} // namespace arcstd::system
