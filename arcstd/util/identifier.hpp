#pragma once

#include "arcstd/export.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace arcstd::util
{

///
/// Name of letters, digits and '_' only, never empty, e.g. the name of a cache. So it reads the
/// same as a key, in a path and in a file name, whoever chose it.
///
class ARCSTD_CORE_EXPORT identifier final
{
  // Variables
  std::string value_;

public: // Constants
  static constexpr auto characters =
    std::string_view{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"};

public: // Static interface
  ///
  /// \return the identifier of \p text, or std::nullopt if it is empty
  ///
  [[nodiscard]] static auto from(std::string_view text) -> std::optional<identifier>;

public: // Structors
  ///
  /// Characters of \p text other than `characters` are replaced by '_'.
  /// \throw util::exception if \p text is empty, \see from
  ///
  explicit identifier(std::string_view text);

public: // Operators
  auto operator<=>(const identifier& other) const = default;
  auto operator==(const identifier& other) const -> bool = default;
  auto operator==(std::string_view other) const -> bool;

public: // Accessors
  [[nodiscard]] auto string() const -> const std::string&;
};

} // namespace arcstd::util
