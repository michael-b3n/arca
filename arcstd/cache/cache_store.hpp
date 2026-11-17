#pragma once

#include "arcstd/export.hpp"
#include "arcstd/lua/lua_engine.hpp"
#include "arcstd/util/identifier.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace arcstd::cache
{

class cache_database;

///
/// Keeps text values of the scripts beyond the run of the app, in a database file per cache name.
/// Registered for scripts as cache.get(name, key) and cache.set(name, key, value), a script names
/// its own cache, e.g. after itself.
///
class ARCSTD_CACHE_EXPORT cache_store final
{
  // Constants
  static constexpr auto folder_name = "cache";

  // Variables
  const std::shared_ptr<lua::lua_engine> lua_engine_;
  const std::filesystem::path folder_;
  mutable std::mutex mtx_;
  std::map<std::string, std::unique_ptr<cache_database>> caches_;
  lua::lua_engine::registration_type registrations_;

public: // Structors
  ///
  /// Caches in the folder "cache" of \p data_folder, with their functions for the scripts of
  /// \p lua_engine, none without one.
  ///
  cache_store(
    const std::filesystem::path& data_folder, std::shared_ptr<lua::lua_engine> lua_engine
  );
  ~cache_store() noexcept;

public: // Modifiers
  ///
  /// \return the value of \p key in the cache \p name, std::nullopt if it is not cached
  ///
  [[nodiscard]] auto get(const util::identifier& name, const std::string& key)
    -> std::optional<std::string>;

  ///
  /// Cache \p value for \p key in the cache \p name, std::nullopt removes it.
  ///
  auto set(
    const util::identifier& name, const std::string& key, const std::optional<std::string>& value
  ) -> void;

  ///
  /// Cache each of \p values for its key in the cache \p name, all of them or none.
  ///
  auto set_all(const util::identifier& name, const std::map<std::string, std::string>& values)
    -> void;

  ///
  /// Remove all values of the cache \p name.
  ///
  auto clear(const util::identifier& name) -> void;

private: // Implementation
  [[nodiscard]] auto cache(const util::identifier& name) -> cache_database&;
};

} // namespace arcstd::cache
