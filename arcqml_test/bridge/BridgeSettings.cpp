#include "qml_test_utils/qt_strings.hpp"
#include "test_utils/settings.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/bridge/BridgeSettings.hpp"
#include "arcqml/bridge/SettingBinding.hpp"

#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QVariant>
#include <QVariantList>

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace arcqml
{
namespace
{

// Typedefs
using SettingStore = arcstd::setting::setting_store;

///
/// \return true if the setting \p T of \p path in \p store has a validator \p V
///
template<typename T, typename V>
[[nodiscard]] auto hasValidator(const SettingStore& store, const std::string& path) -> bool
{
  const auto* const setting = arcstd::test_utils::type_erased_setting<T>(store, path);
  return std::holds_alternative<typename V::sptr_type>(setting->validator);
}

} // namespace

TEST_CASE("BridgeSettings_is_the_instance_qml_operates_on", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"BridgeSettings_is_the_instance_qml_operates_on"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  CHECK(BridgeSettings::instance() == nullptr);
  CHECK(BridgeSettings::create(nullptr, nullptr) == nullptr);
  {
    auto registry = BridgeSettings{store};
    CHECK(BridgeSettings::instance() == &registry);
    CHECK(BridgeSettings::create(nullptr, nullptr) == &registry);
    CHECK(registry.settingStore() == store);
    {
      // A second one is not the instance
      const auto second = BridgeSettings{store};
      CHECK(BridgeSettings::instance() == &registry);
    }
    CHECK(BridgeSettings::instance() == &registry);
  }
  CHECK(BridgeSettings::instance() == nullptr);
}

TEST_CASE("BridgeSettings_creates_one_binding_per_path", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"BridgeSettings_creates_one_binding_per_path"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  auto registry = BridgeSettings{store};
  CHECK_FALSE(registry.contains(QStringLiteral("ui.size")));

  auto* const size = registry.binding(QStringLiteral("ui.size"), 3);
  REQUIRE(size != nullptr);
  CHECK(size->bound());
  CHECK(size->path() == QStringLiteral("ui.size"));
  CHECK(registry.contains(QStringLiteral("ui.size")));
  // Every caller shares it, the default value of the first one wins
  CHECK(registry.binding(QStringLiteral("ui.size"), 3) == size);
  CHECK(registry.binding(QStringLiteral("ui.size"), 4) == size);
  CHECK(size->value() == QVariant{3});
  // No path, no binding
  CHECK(registry.binding(QString{}, 3) == nullptr);
}

TEST_CASE("BridgeSettings_creates_a_setting_with_a_validator", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"BridgeSettings_creates_a_setting_with_a_validator"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  auto registry = BridgeSettings{store};

  auto* const name = registry.binding(
    QStringLiteral("ui.name"), QStringLiteral("a"), QVariantList{QStringLiteral("a"), "b"}
  );
  REQUIRE(name != nullptr);
  using NameList = SettingStore::validator_list_type_erased_type<std::string>;
  CHECK(hasValidator<std::string, NameList>(*store, "ui.name"));

  auto* const size = registry.binding(QStringLiteral("ui.size"), 3, 0, 10);
  REQUIRE(size != nullptr);
  using SizeRange = SettingStore::validator_range_type_erased_type<std::int64_t>;
  CHECK(hasValidator<std::int64_t, SizeRange>(*store, "ui.size"));

  // Without one a setting takes any value
  REQUIRE(registry.binding(QStringLiteral("ui.free"), 3) != nullptr);
  CHECK(hasValidator<std::int64_t, SettingStore::validator_unbound_type>(*store, "ui.free"));
}

} // namespace arcqml
