#include "arcstd/system/filesystem.hpp"

#include <boost/dll.hpp>

#include <filesystem>

namespace arcstd::system
{

///
///
auto filesystem::executable_location() -> std::filesystem::path
{
  return boost::dll::program_location().string();
}

///
///
auto filesystem::executable_folder() -> std::filesystem::path
{
  return boost::dll::program_location().parent_path().string();
}

} // namespace arcstd::system
