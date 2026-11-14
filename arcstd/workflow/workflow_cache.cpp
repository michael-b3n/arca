#include "arcstd/workflow/workflow_cache.hpp"
#include "arcstd/core/core_cache.hpp"
#include "arcstd/util/identifier.hpp"

#include <format>
#include <optional>
#include <utility>

namespace arcstd::workflow
{

///
///
workflow_cache::workflow_cache(
  std::shared_ptr<setting::setting_store> setting_store, std::shared_ptr<lua::lua_engine> lua_engine
)
  : lua_engine_{std::move(lua_engine)}
  , folder_{setting_store->data_folder() / folder_name}
{
  registrations_
    ///
    /// \lua Reads a text value from a cache. A cache keeps its values after the app closes. Each cache is the SQLite file
    /// `cache/<name>.sqlite` in the local data folder, and other programs may read and change it. A cache that fails is
    /// logged and holds nothing.
    /// \param name the cache, made of letters, digits and `_`. Each script uses its own cache.
    /// \param key the key of the value
    /// \return the value, or `nil` if there is none
    ///
    << lua_engine_->register_function(
         "cache.get", [this](const std::string& name, const std::string& key) { return get(util::identifier{name}, key); }
       )
    ///
    /// \lua Writes a text value to a cache, \see `cache.get`.
    /// \param name the cache
    /// \param key the key of the value
    /// \param value the value, `nil` removes the key
    ///
    << lua_engine_->register_function(
         "cache.set",
         [this](const std::string& name, const std::string& key, std::optional<std::string> value)
         { set(util::identifier{name}, key, std::move(value)); }
       )
    ///
    /// \lua Writes many text values to a cache at once, all of them or none, \see `cache.get`.
    /// \param name the cache
    /// \param values the values by key
    ///
    << lua_engine_->register_function(
         "cache.set_all",
         [this](const std::string& name, std::map<std::string, std::string> values) { set_all(util::identifier{name}, values); }
       )
    ///
    /// \lua Removes all values of a cache, \see `cache.get`.
    /// \param name the cache
    ///
    << lua_engine_->register_function("cache.clear", [this](const std::string& name) { clear(util::identifier{name}); });
}

///
///
workflow_cache::~workflow_cache() noexcept = default;

///
///
auto workflow_cache::get(const util::identifier& name, const std::string& key) -> std::optional<std::string>
{
  return cache(name).get(key);
}

///
///
auto workflow_cache::set(const util::identifier& name, const std::string& key, std::optional<std::string> value) -> void
{
  cache(name).set(key, std::move(value));
}

///
///
auto workflow_cache::set_all(const util::identifier& name, const std::map<std::string, std::string>& values) -> void
{
  cache(name).set_all(values);
}

///
///
auto workflow_cache::clear(const util::identifier& name) -> void
{
  cache(name).clear();
}

///
///
auto workflow_cache::cache(const util::identifier& name) -> core::core_cache&
{
  const auto lock = std::scoped_lock{mtx_};
  auto& cache = caches_[name.string()];
  if(!cache)
  {
    cache = std::make_unique<core::core_cache>(folder_ / std::format("{}.sqlite", name.string()));
  }
  return *cache;
}

} // namespace arcstd::workflow
