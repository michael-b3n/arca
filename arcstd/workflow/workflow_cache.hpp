#pragma once

#include "arcstd/export.hpp"
#include "arcstd/lua/lua_engine.hpp"
#include "arcstd/setting/setting_store.hpp"
#include "arcstd/util/identifier.hpp"
#include "arcstd/workflow/workflow_base.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace arcstd::core
{
class core_cache;
} // namespace arcstd::core

namespace arcstd::workflow
{

///
/// Workflow cache, keeps text values of the scripts beyond the run of the app, in a database file per cache name.
/// Registered for scripts as workflow.cache.get(name, key) and workflow.cache.set(name, key, value), a script names
/// its own cache, e.g. after itself.
///
class ARCSTD_EXPORT workflow_cache final : public workflow_base<void>
{
  // Constants
  static constexpr auto folder_name = "cache";

  // Variables
  const std::shared_ptr<lua::lua_engine> lua_engine_;
  const std::filesystem::path folder_;
  mutable std::mutex mtx_;
  std::map<std::string, std::unique_ptr<core::core_cache>> caches_;
  lua::lua_engine::registration_type registrations_;

public: // Structors
  workflow_cache(std::shared_ptr<setting::setting_store> setting_store, std::shared_ptr<lua::lua_engine> lua_engine);
  ~workflow_cache() noexcept override;

public: // Modifiers
  ///
  /// \return the value of \p key in the cache \p name, std::nullopt if it is not cached
  ///
  [[nodiscard]] auto get(const util::identifier& name, const std::string& key) -> std::optional<std::string>;

  ///
  /// Cache \p value for \p key in the cache \p name, std::nullopt removes it.
  ///
  auto set(const util::identifier& name, const std::string& key, std::optional<std::string> value) -> void;

  ///
  /// Cache each of \p values for its key in the cache \p name, all of them or none.
  ///
  auto set_all(const util::identifier& name, const std::map<std::string, std::string>& values) -> void;

  ///
  /// Remove all values of the cache \p name.
  ///
  auto clear(const util::identifier& name) -> void;

private: // Implementation
  [[nodiscard]] auto cache(const util::identifier& name) -> core::core_cache&;
};

} // namespace arcstd::workflow
