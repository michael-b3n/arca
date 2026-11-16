#pragma once

#include "arcstd/export.hpp"

#include <filesystem>

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
};

} // namespace arcstd::system
