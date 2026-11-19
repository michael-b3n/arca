#pragma once

#include <arcstd/lua/lua_engine.hpp>

#include <utility>
#include <vector>

namespace arcstd::test_utils
{

///
/// Keeps the registrations of a test, so what it registers lasts as long as this object.
///
class registrations final
{
  // Variables
  std::vector<lua::lua_engine::registration_type> kept_;

public: // Operators
  ///
  /// Keep \p registration.
  /// \return true if it registered something
  ///
  auto operator()(lua::lua_engine::registration_type registration) -> bool;
};

///
///
inline auto registrations::operator()(lua::lua_engine::registration_type registration) -> bool
{
  kept_.push_back(std::move(registration));
  return static_cast<bool>(kept_.back());
}

} // namespace arcstd::test_utils
