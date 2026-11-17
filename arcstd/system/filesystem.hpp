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
  /// \return the path of the running executable
  ///
  [[nodiscard]] static auto executable_location() -> std::filesystem::path;

  ///
  /// \return the folder of the running executable
  ///
  [[nodiscard]] static auto executable_folder() -> std::filesystem::path;
};

} // namespace arcstd::system
