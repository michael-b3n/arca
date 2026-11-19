#include "test_utils/events.hpp"
#include "test_utils/qt_strings.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/bridge/BridgeSettings.hpp"
#include "arcqml/bridge/SettingBinding.hpp"

#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QObject>
#include <QPointF>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <variant>

namespace arcqml
{
namespace
{

///
/// \return the type erased setting \p T of \p path in \p store, which must exist
///
template<typename T>
[[nodiscard]] auto erased(const arcstd::setting::setting_store& store, const std::string& path)
  -> arcstd::setting::setting_store::setting_type_erased_non_owning_ptr_type<T>
{
  const auto setting = store.type_erased_setting(path);
  REQUIRE(setting);
  return std::get<arcstd::setting::setting_store::setting_type_erased_non_owning_ptr_type<T>>(
    *setting
  );
}

} // namespace

TEST_CASE("SettingBinding_without_a_registry_keeps_its_default_value", "[bridge]")
{
  REQUIRE(BridgeSettings::instance() == nullptr);
  auto binding = SettingBinding{QStringLiteral("ui.size"), 3};
  CHECK_FALSE(binding.bound());
  CHECK(binding.path() == QStringLiteral("ui.size"));
  CHECK(binding.defaultValue() == QVariant{3});
  binding.setValue(4);
  CHECK(binding.value() == QVariant{3});
}

TEST_CASE("SettingBinding_binds_the_value_to_the_setting", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingBinding_binds_the_value_to_the_setting"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto registry = BridgeSettings{store};
  auto binding = SettingBinding{QStringLiteral("ui.size"), 3};
  auto changed = 0;
  QObject::connect(&binding, &SettingBinding::valueChanged, [&] { ++changed; });
  REQUIRE(binding.bound());
  CHECK(binding.value() == QVariant{3});
  // QML numbers make 64 bit settings
  auto* const size = erased<std::int64_t>(*store, "ui.size");
  CHECK(size->value() == 3);

  binding.setValue(5);
  CHECK(size->value() == 5);
  CHECK(binding.value() == QVariant{5});
  CHECK(test_utils::processEventsUntil([&] { return changed == 1; }));

  // A value changed elsewhere, e.g. by a script
  CHECK(size->value(6));
  CHECK(test_utils::processEventsUntil([&] { return changed == 2; }));
  CHECK(binding.value() == QVariant{6});
}

TEST_CASE("SettingBinding_takes_the_values_of_its_validator", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingBinding_takes_the_values_of_its_validator"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto registry = BridgeSettings{store};

  auto size = SettingBinding{QStringLiteral("ui.size"), 3, 0, 10};
  auto sizeChanged = 0;
  QObject::connect(&size, &SettingBinding::valueChanged, [&] { ++sizeChanged; });
  REQUIRE(size.bound());
  // A corrected value tells QML at once to show the value of the setting
  size.setValue(20);
  CHECK(sizeChanged == 1);
  CHECK(size.value() == QVariant{9});

  auto name = SettingBinding{
    QStringLiteral("ui.name"), QStringLiteral("a"), QVariantList{QStringLiteral("a"), "b"}
  };
  auto nameChanged = 0;
  QObject::connect(&name, &SettingBinding::valueChanged, [&] { ++nameChanged; });
  REQUIRE(name.bound());
  name.setValue(QStringLiteral("c"));
  CHECK(nameChanged == 1);
  CHECK(name.value() == QVariant{QStringLiteral("a")});
  name.setValue(QStringLiteral("b"));
  CHECK(name.value() == QVariant{QStringLiteral("b")});
}

TEST_CASE("SettingBinding_stays_unbound_without_a_fitting_setting", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingBinding_stays_unbound_without_a_fitting_setting"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto registry = BridgeSettings{store};
  std::ignore = store->create_setting("ui.name", std::string{"name"});

  // No path, a default value of no setting type, or one of another type than the setting has
  CHECK_FALSE(SettingBinding{QString{}, 3}.bound());
  CHECK_FALSE(
    SettingBinding{
      QStringLiteral("ui.point"), QPointF{1.0, 2.0}
  }
      .bound()
  );
  const auto name = SettingBinding{QStringLiteral("ui.name"), 3};
  CHECK_FALSE(name.bound());
  CHECK(name.value() == QVariant{3});
  // A validator that does not fit the default value
  CHECK_FALSE(SettingBinding{QStringLiteral("ui.flag"), true, 0, 1}.bound());
  CHECK_FALSE(SettingBinding{QStringLiteral("ui.count"), 1, QVariantList{"x"}}.bound());
  CHECK_FALSE(store->type_erased_setting("ui.point").has_value());
}

} // namespace arcqml
