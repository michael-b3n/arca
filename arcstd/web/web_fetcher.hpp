#pragma once

#include "arcstd/export.hpp"
#include "arcstd/lua/lua_engine.hpp"
#include "arcstd/setting/setting_store.hpp"

#include <chrono>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace arcstd::web
{

///
/// Settings of the web fetcher.
///
class ARCSTD_EXPORT web_fetcher_settings final
{
  // Variables
  const std::shared_ptr<setting::setting_store> setting_store_;

public: // Structors
  explicit web_fetcher_settings(std::shared_ptr<setting::setting_store> setting_store);

public: // Variables
  const setting::setting_store::setting_non_owning_ptr_type<std::chrono::seconds> retry_pause;
};

///
/// Web fetcher, fetches web pages. Registered for scripts as web.fetch(url).
///
class ARCSTD_EXPORT web_fetcher final
{
  // Typedefs
  struct failure_t final
  {
    std::string reason;
    std::chrono::steady_clock::time_point retry_at;
  };

  // Variables
  const web_fetcher_settings settings_;
  const std::shared_ptr<lua::lua_engine> lua_engine_;
  mutable std::mutex mtx_;
  mutable std::map<std::string, failure_t> failures_;
  lua::lua_engine::registration_type registrations_;

public: // Typedefs
  using page_type = std::expected<std::string, std::string>;

public: // Structors
  ///
  /// Construct the fetcher with its settings in \p setting_store and its function for the scripts of \p lua_engine,
  /// none without one.
  ///
  web_fetcher(std::shared_ptr<setting::setting_store> setting_store, std::shared_ptr<lua::lua_engine> lua_engine);
  ~web_fetcher() noexcept;

public: // Accessors
  ///
  /// \return the settings of the fetcher
  ///
  [[nodiscard]] auto settings() const -> const web_fetcher_settings&;

public: // Operations
  ///
  /// \return the web page at \p url, or why it failed: the name of the error.
  /// Blocks until it is fetched. During the retry pause a failed url gets the same failure without a request.
  ///
  [[nodiscard]] auto fetch(std::string_view url) const -> page_type;
};

} // namespace arcstd::web
