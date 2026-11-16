#pragma once

#include "arcstd/export.hpp"
#include "arcstd/util/string.hpp"

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace arcstd::util
{

///
/// Path of sections separated by '.', e.g. the path of a setting, a Lua node or a script manifest.
/// Empty sections are left out, so "a..b." is the path "a.b". Built from sections, each is read the same way.
///
class ARCSTD_EXPORT path final
{
  // Typedefs
  using sections_type = std::vector<std::string>;

  // Constants
  static constexpr auto delimiter = '.';

  // Variables
  sections_type sections_;

public: // Structors
  path() = default;
  // NOLINTBEGIN(misc-explicit-constructor) a path is given as its string or its sections
  path(const util::string::string_view_type auto& p);
  path(const std::ranges::input_range auto& sections)
    requires(!util::string::string_view_type<decltype(sections)>);
  // NOLINTEND(misc-explicit-constructor)

public: // Operators
  auto operator<=>(const path& other) const = default;

  ///
  /// \return this path followed by the sections of \p other
  ///
  [[nodiscard]] auto operator/(const path& other) const -> path;

public: // Accessors
  ///
  /// \return the sections joined by the delimiter
  ///
  [[nodiscard]] auto string() const -> std::string;

  ///
  /// \return true if the path has no sections
  ///
  [[nodiscard]] auto empty() const -> bool;

  ///
  /// \return the sections of the path
  ///
  [[nodiscard]] auto sections() const -> const sections_type&;

  ///
  /// \return true if \p other starts with all sections of this path, also if both are equal
  ///
  [[nodiscard]] auto contains(const path& other) const -> bool;

private: // Implementation
  [[nodiscard]] static auto normalize(std::string_view p) -> sections_type;
};

///
///
path::path(const util::string::string_view_type auto& p)
  : sections_{normalize(std::string_view{p})}
{
}

///
///
path::path(const std::ranges::input_range auto& sections)
  requires(!util::string::string_view_type<decltype(sections)>)
  : sections_{normalize(util::string::join(std::ranges::to<sections_type>(sections), delimiter))}
{
}

} // namespace arcstd::util
