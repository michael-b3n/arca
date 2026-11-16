#include "arcstd/cache/cache_store.hpp"
#include "arcstd/cache/cache_database.hpp"
#include "arcstd/util/identifier.hpp"

#include <format>
#include <optional>
#include <utility>

namespace arcstd::cache
{

///
///
cache_store::cache_store(const std::filesystem::path& data_folder, std::shared_ptr<lua::lua_engine> lua_engine)
  : lua_engine_{std::move(lua_engine)}
  , folder_{data_folder / folder_name}
{
  if(!lua_engine_)
  {
    return;
  }

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
         [this](const std::string& name, const std::string& key, const std::optional<std::string>& value)
         { set(util::identifier{name}, key, value); }
       )
    ///
    /// \lua Writes many text values to a cache at once, all of them or none, \see `cache.get`.
    /// \param name the cache
    /// \param values the values by key
    ///
    << lua_engine_->register_function(
         "cache.set_all",
         [this](const std::string& name, const std::map<std::string, std::string>& values)
         { set_all(util::identifier{name}, values); }
       )
    ///
    /// \lua Removes all values of a cache, \see `cache.get`.
    /// \param name the cache
    ///
    << lua_engine_->register_function("cache.clear", [this](const std::string& name) { clear(util::identifier{name}); });
}

///
///
cache_store::~cache_store() noexcept = default;

///
///
auto cache_store::get(const util::identifier& name, const std::string& key) -> std::optional<std::string>
{
  return cache(name).get(key);
}

///
///
auto cache_store::set(const util::identifier& name, const std::string& key, const std::optional<std::string>& value) -> void
{
  cache(name).set(key, value);
}

///
///
auto cache_store::set_all(const util::identifier& name, const std::map<std::string, std::string>& values) -> void
{
  cache(name).set_all(values);
}

///
///
auto cache_store::clear(const util::identifier& name) -> void
{
  cache(name).clear();
}

///
///
auto cache_store::cache(const util::identifier& name) -> cache_database&
{
  const auto lock = std::scoped_lock{mtx_};
  auto& cache = caches_[name.string()];
  if(!cache)
  {
    cache = std::make_unique<cache_database>(folder_ / std::format("{}.sqlite", name.string()));
  }
  return *cache;
}

} // namespace arcstd::cache
