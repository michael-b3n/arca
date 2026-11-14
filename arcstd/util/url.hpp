#pragma once

#include "arcstd/export.hpp"

#include <string_view>

namespace arcstd::util::url
{

///
/// \return true if \p url is a valid url of a web page, one of http or https
///
[[nodiscard]] ARCSTD_EXPORT auto is_web_url(std::string_view url) -> bool;

} // namespace arcstd::util::url
