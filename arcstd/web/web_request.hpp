#pragma once

#if !defined(ARCSTD_INTERNAL)
  #error "arcstd/web/web_request.hpp is a private header of arcstd"
#endif

#include <expected>
#include <string>
#include <string_view>

namespace arcstd::web
{

///
/// Web request, fetches the content of a web page with libcurl.
///
class web_request final
{
public: // Typedefs
  ///
  /// Error codes for web content fetching operations.
  ///
  enum class error_code
  {
    init_failed,    ///< Failed to initialize curl
    request_failed, ///< HTTP request failed
    not_found,      ///< HTTP 404 or 410, the server has no such page
    invalid_url,    ///< Invalid URL provided
    timeout,        ///< Request timed out
    unknown         ///< Unknown error occurred
  };

public: // Structors
  web_request() = default;
  ~web_request() noexcept = default;

public: // Operations
  ///
  /// \return Content of the webpage, or an error code
  ///
  [[nodiscard]] static auto fetch(std::string_view url) -> std::expected<std::string, error_code>;
};

} // namespace arcstd::web
