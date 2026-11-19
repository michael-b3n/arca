#pragma once

#include <source_location>
#include <string_view>

namespace arcstd::util
{

///
/// \return the name of the function of \p loc, without its namespace and its parameters
///
[[nodiscard]] constexpr auto filter_function_name(const std::source_location& loc)
  -> std::string_view
{
  const std::string_view func_name = loc.function_name();
  const auto last_braces_pos = func_name.find_first_of('(');
  const std::string_view func_name_without_braces =
    (last_braces_pos != std::string_view::npos) ? func_name.substr(0, last_braces_pos) : func_name;
  const auto colon_pos = func_name_without_braces.find_last_of(':');
  const std::string_view pretty_func_name = (colon_pos != std::string_view::npos)
                                              ? func_name_without_braces.substr(colon_pos + 1)
                                              : func_name_without_braces;
  return pretty_func_name;
}

///
/// \return the name of the file of \p loc, without its folder and its extension
///
[[nodiscard]] constexpr auto filter_file_name(const std::source_location& loc) -> std::string_view
{
  const std::string_view file_name = loc.file_name();
  const auto last_slash_pos = file_name.find_last_of("/\\");
  const std::string_view file_name_with_ext =
    (last_slash_pos != std::string_view::npos) ? file_name.substr(last_slash_pos + 1) : file_name;
  const auto dot_pos = file_name_with_ext.find_last_of('.');
  const std::string_view pretty_file_name = (dot_pos != std::string_view::npos)
                                              ? file_name_with_ext.substr(0, dot_pos)
                                              : file_name_with_ext;
  return pretty_file_name;
}

///
/// \return the name of the folder of the file of \p loc, without the path above it
///
[[nodiscard]] constexpr auto filter_folder_name(const std::source_location& loc) -> std::string_view
{
  const std::string_view file_name = loc.file_name();
  const auto last_slash_pos = file_name.find_last_of("/\\");
  const auto parent_folder_end = (last_slash_pos != std::string_view::npos)
                                   ? file_name.substr(0, last_slash_pos).find_last_of("/\\")
                                   : std::string_view::npos;
  const std::string_view pretty_folder_name =
    (parent_folder_end != std::string_view::npos)
      ? file_name.substr(parent_folder_end + 1, last_slash_pos - (parent_folder_end + 1))
      : "unknown";
  return pretty_folder_name;
}

} // namespace arcstd::util
