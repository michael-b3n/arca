#pragma once

#include "arcstd/export.hpp"
#include "arcstd/setting/setting_store.hpp"
#include "arcstd/workflow/workflow_base.hpp"
#include "arcstd/workflow/workflow_script.hpp"
#include "arcstd/workflow/workflow_settings_base.hpp"

#include <chrono>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace arcstd::workflow
{

///
/// Settings corresponding to workflow web.
///
class ARCSTD_EXPORT workflow_web_settings final : public workflow_settings_base
{
public: // Structors
  workflow_web_settings(std::shared_ptr<setting::setting_store> setting_store);
  ~workflow_web_settings() noexcept override = default;

public: // Variables
  const setting_type<std::chrono::seconds> retry_pause;
};

///
/// Workflow web, fetches web pages.
///
class ARCSTD_EXPORT workflow_web final : public workflow_base<workflow_web_settings>
{
  // Typedefs
  struct failure_t final
  {
    std::string reason;
    std::chrono::steady_clock::time_point retry_at;
  };

  // Variables
  const std::shared_ptr<workflow_script> workflow_script_;
  mutable std::mutex mtx_;
  mutable std::map<std::string, failure_t> failures_;
  workflow_script::registration_type registrations_;

public: // Typedefs
  using page_type = std::expected<std::string, std::string>;

public: // Structors
  workflow_web(std::shared_ptr<setting::setting_store> setting_store, std::shared_ptr<workflow_script> workflow_script);
  ~workflow_web() noexcept override;

public: // Operations
  ///
  /// \return the web page at \p url, or why it failed: the name of the error.
  /// Blocks until it is fetched. During the retry pause a failed url gets the same failure without a request.
  ///
  [[nodiscard]] auto fetch(std::string_view url) const -> page_type;
};

} // namespace arcstd::workflow
