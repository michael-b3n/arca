#pragma once

#ifndef ARCSTD_INTERNAL
  #error "private header of arcstd"
#endif

#include <expected>
#include <string>
#include <string_view>

namespace arcstd::web
{

///
/// Fetches the content of a web page with libcurl.
///
class web_request final
{
public: // Typedefs
  enum class error_code
  {
    init_failed,
    request_failed,
    not_found, ///< HTTP 404 or 410, the server has no such page
    invalid_url,
    timeout,
    unknown,
  };

public: // Operations
  ///
  /// \return the content of the web page at \p url, or why it failed
  ///
  [[nodiscard]] static auto fetch(std::string_view url) -> std::expected<std::string, error_code>;
};

} // namespace arcstd::web
