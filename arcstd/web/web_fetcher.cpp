#include "arcstd/web/web_fetcher.hpp"
#include "arcstd/util/enum.hpp"
#include "arcstd/util/exception.hpp"
#include "arcstd/web/web_request.hpp"

#include <utility>

namespace arcstd::web
{

///
///
web_fetcher_settings::web_fetcher_settings(std::shared_ptr<setting::setting_store> setting_store)
  : setting_store_{std::move(setting_store)}
  , retry_pause{setting_store_->create_setting(
      "web.retry_pause",
      std::chrono::seconds{60},
      std::make_shared<setting::setting_validator_range<std::chrono::seconds>>(std::chrono::seconds{0}, std::chrono::hours{24})
    )}
{
}

///
///
web_fetcher::web_fetcher(std::shared_ptr<setting::setting_store> setting_store, std::shared_ptr<lua::lua_engine> lua_engine)
  : settings_{std::move(setting_store)}
  , lua_engine_{std::move(lua_engine)}
{
  if(!lua_engine_)
  {
    return;
  }

  registrations_
    ///
    /// \lua How long a failed url is not requested again, `Web > Pause after a failure`. With 0 s a script may
    /// request a failed url again right away.
    ///
    << lua_engine_->register_setting(*settings().retry_pause)
    ///
    /// \lua Fetches a web page. The script waits for the page, and other scripts wait too. During
    /// `Web > Pause after a failure` a failed url is not requested again, it returns the same failure.
    /// \param url the address of the page, http or https
    /// \return the content of the page, or `nil` and the reason: `invalid_url`, `not_found` (HTTP 404 or 410),
    /// `timeout` (after 30 s), `init_failed` or `request_failed`
    ///
    << lua_engine_->register_function("web.fetch", [this](const std::string& url) { return fetch(url); });
}

///
///
web_fetcher::~web_fetcher() noexcept = default;

///
///
auto web_fetcher::settings() const -> const web_fetcher_settings&
{
  return settings_;
}

///
///
auto web_fetcher::fetch(const std::string_view url) const -> page_type
{
  using clock_type = std::chrono::steady_clock;
  try
  {
    {
      const auto lock = std::scoped_lock{mtx_};
      std::erase_if(failures_, [now = clock_type::now()](const auto& failure) { return failure.second.retry_at <= now; });
      if(const auto failed = failures_.find(std::string{url}); failed != failures_.cend())
      {
        return std::unexpected{failed->second.reason};
      }
    }
    // Without the lock, the request may take until its timeout
    auto page = web_request::fetch(url).transform_error([](const auto error) { return std::string{util::enum_name(error)}; });
    if(!page)
    {
      const auto lock = std::scoped_lock{mtx_};
      failures_.insert_or_assign(
        std::string{url}, failure_t{.reason = page.error(), .retry_at = clock_type::now() + settings().retry_pause->value()}
      );
    }
    return page;
  }
  catch(...)
  {
    return std::unexpected{util::exception_report()};
  }
}

} // namespace arcstd::web
