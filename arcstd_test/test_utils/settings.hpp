#pragma once

#include <arcstd/setting/setting.hpp>
#include <arcstd/setting/setting_store.hpp>
#include <arcstd/signal/adapter.hpp>
#include <arcstd/signal/common.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>

namespace arcstd::test_utils
{

///
/// \return the type erased setting of type \p T that \p setting points to, the test fails if it
/// points to one of another type
///
template<setting::underlying_setting_type_erased_type T>
[[nodiscard]] auto type_erased_setting(
  const setting::setting_store::setting_type_erased_non_owning_ptr_variant_type& setting
) -> setting::setting_store::setting_type_erased_non_owning_ptr_type<T>
{
  using pointer_type = setting::setting_store::setting_type_erased_non_owning_ptr_type<T>;
  REQUIRE(std::holds_alternative<pointer_type>(setting));
  return std::get<pointer_type>(setting);
}

///
/// \return the type erased setting \p path of \p store, of type \p T, the test fails if there is
/// none of that type
///
template<setting::underlying_setting_type_erased_type T>
[[nodiscard]] auto type_erased_setting(const setting::setting_store& store, const std::string& path)
  -> setting::setting_store::setting_type_erased_non_owning_ptr_type<T>
{
  const auto setting = store.type_erased_setting(path);
  REQUIRE(setting);
  return type_erased_setting<T>(*setting);
}

///
/// Counts the signals of a setting while it lives.
///
class setting_signal_counter final
{
  // Variables
  int value_changed_{0};
  int validator_changed_{0};
  const signal::scoped_connection_type value_connection_;
  const signal::scoped_connection_type validator_connection_;

public: // Structors
  explicit setting_signal_counter(const signal::adapter<setting::setting_signals>& setting);
  ~setting_signal_counter() noexcept = default;

  setting_signal_counter(const setting_signal_counter&) = delete;
  auto operator=(const setting_signal_counter&) -> setting_signal_counter& = delete;

public: // Accessors
  ///
  /// \return how often the value changed
  ///
  [[nodiscard]] auto value_changed() const -> int;

  ///
  /// \return how often the validator changed
  ///
  [[nodiscard]] auto validator_changed() const -> int;
};

///
///
inline setting_signal_counter::setting_signal_counter(
  const signal::adapter<setting::setting_signals>& setting
)
  : value_connection_{
      setting.connect(&setting::setting_signals::value_changed, [this] { ++value_changed_; })
    }
  , validator_connection_{setting.connect(
      &setting::setting_signals::validator_changed, [this] { ++validator_changed_; }
    )}
{
}

///
///
inline auto setting_signal_counter::value_changed() const -> int
{
  return value_changed_;
}

///
///
inline auto setting_signal_counter::validator_changed() const -> int
{
  return validator_changed_;
}

} // namespace arcstd::test_utils
