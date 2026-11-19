#include "test_utils/events.hpp"
#include "test_utils/qt_strings.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/model/SettingsListModel.hpp"

#include <arcstd/setting/setting_store.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace arcqml
{
namespace
{

enum class mode
{
  light,
  dark,
};

///
/// \return the value of \p role in \p row of \p model
///
[[nodiscard]] auto roleData(const SettingsListModel& model, const int row, const int role)
  -> QVariant
{
  return model.data(model.index(row), role);
}

///
/// \return the paths of the rows of \p model
///
[[nodiscard]] auto paths(const SettingsListModel& model) -> QStringList
{
  auto result = QStringList{};
  for(auto row = 0; row < model.rowCount(); ++row)
  {
    result.append(roleData(model, row, SettingsListModel::PathRole).toString());
  }
  return result;
}

} // namespace

TEST_CASE("SettingsListModel_lists_the_settings_but_the_internal_ones", "[model]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingsListModel_lists_the_settings_but_the_internal_ones"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto range =
    std::make_shared<arcstd::setting::setting_validator_range<std::int32_t>>(0, 10);
  std::ignore = store->create_setting("ui.size", std::int32_t{3}, "px", range);
  std::ignore = store->create_setting("internal.window.x", std::int32_t{0});
  std::ignore = store->create_setting("ui.mode", mode::dark);
  std::ignore = store->create_setting("ui.limit", std::optional<double>{});
  std::ignore = store->create_setting("ui.pauses", std::vector<std::chrono::seconds>{});
  std::ignore = store->create_setting("ui.flag", true);
  std::ignore = store->create_setting("ui.folder", std::filesystem::path{"a"});
  const auto model = SettingsListModel{store};

  REQUIRE(
    paths(model) ==
    QStringList{"ui.size", "ui.mode", "ui.limit", "ui.pauses", "ui.flag", "ui.folder"}
  );
  CHECK(model.rowCount(model.index(0)) == 0);
  CHECK(model.roleNames().value(SettingsListModel::ListValidatorDataRole) == "listValidatorData");
  CHECK(model.flags(model.index(0)) == Qt::NoItemFlags);

  // A number in a range, with a postfix
  CHECK(
    roleData(model, 0, SettingsListModel::CategoriesRole).toStringList() ==
    QStringList{"ui", "size"}
  );
  CHECK(roleData(model, 0, SettingsListModel::ValueTypeRole) == SettingsListModel::IntValueType);
  CHECK(
    roleData(model, 0, SettingsListModel::WrapperTypeRole) == SettingsListModel::NoneWrapperType
  );
  CHECK(
    roleData(model, 0, SettingsListModel::ValidatorTypeRole) ==
    SettingsListModel::RangeValidatorType
  );
  CHECK(roleData(model, 0, SettingsListModel::ValueRole) == QVariant{3});
  CHECK(roleData(model, 0, SettingsListModel::PostfixRole) == QStringLiteral("px"));
  CHECK(roleData(model, 0, SettingsListModel::ListValidatorDataRole).toList().isEmpty());

  // An enum is a text out of a list
  CHECK(roleData(model, 1, SettingsListModel::ValueTypeRole) == SettingsListModel::StringValueType);
  CHECK(
    roleData(model, 1, SettingsListModel::ValidatorTypeRole) == SettingsListModel::ListValidatorType
  );
  CHECK(roleData(model, 1, SettingsListModel::ValueRole) == QStringLiteral("dark"));
  CHECK(
    roleData(model, 1, SettingsListModel::ListValidatorDataRole).toList() ==
    QVariantList{"light", "dark"}
  );

  // Wrapped values
  CHECK(roleData(model, 2, SettingsListModel::ValueTypeRole) == SettingsListModel::DoubleValueType);
  CHECK(
    roleData(model, 2, SettingsListModel::WrapperTypeRole) == SettingsListModel::OptionalWrapperType
  );
  CHECK(
    roleData(model, 2, SettingsListModel::ValidatorTypeRole) ==
    SettingsListModel::UnboundValidatorType
  );
  CHECK_FALSE(roleData(model, 2, SettingsListModel::ValueRole).isValid());
  CHECK(roleData(model, 3, SettingsListModel::ValueTypeRole) == SettingsListModel::TimeValueType);
  CHECK(
    roleData(model, 3, SettingsListModel::WrapperTypeRole) == SettingsListModel::ListWrapperType
  );
  CHECK(roleData(model, 3, SettingsListModel::PostfixRole) == QStringLiteral("s"));

  CHECK(roleData(model, 4, SettingsListModel::ValueTypeRole) == SettingsListModel::BoolValueType);
  CHECK(roleData(model, 5, SettingsListModel::ValueTypeRole) == SettingsListModel::PathValueType);

  // Neither another role nor another row holds anything
  CHECK_FALSE(roleData(model, 0, Qt::DisplayRole).isValid());
  CHECK_FALSE(roleData(model, 6, SettingsListModel::PathRole).isValid());
}

TEST_CASE("SettingsListModel_writes_the_value_of_a_setting", "[model]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingsListModel_writes_the_value_of_a_setting"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto range =
    std::make_shared<arcstd::setting::setting_validator_range<std::int32_t>>(0, 10);
  const auto size = store->create_setting("ui.size", std::int32_t{3}, range);
  const auto uiMode = store->create_setting("ui.mode", mode::light);
  auto model = SettingsListModel{store};
  auto changed = QList<int>{};
  QObject::connect(
    &model,
    &SettingsListModel::dataChanged,
    [&](const QModelIndex& index) { changed.append(index.row()); }
  );

  CHECK(model.setData(model.index(0), 5, SettingsListModel::ValueRole));
  CHECK(size->value() == 5);
  CHECK(model.setData(model.index(1), QStringLiteral("dark"), SettingsListModel::ValueRole));
  CHECK(uiMode->value() == mode::dark);
  CHECK(changed.isEmpty());

  // A corrected or rejected value tells the view to show the value of the setting again
  CHECK_FALSE(model.setData(model.index(0), 20, SettingsListModel::ValueRole));
  CHECK(size->value() == 9);
  CHECK_FALSE(
    model.setData(model.index(1), QStringLiteral("purple"), SettingsListModel::ValueRole)
  );
  CHECK(uiMode->value() == mode::dark);
  CHECK(changed == QList<int>{0, 1});

  // Nor are other roles or rows written
  CHECK_FALSE(model.setData(model.index(0), QStringLiteral("x"), SettingsListModel::PathRole));
  CHECK_FALSE(model.setData(model.index(2), 1, SettingsListModel::ValueRole));
  CHECK(roleData(model, 0, SettingsListModel::PathRole) == QStringLiteral("ui.size"));
}

TEST_CASE("SettingsListModel_follows_the_setting_store", "[model]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingsListModel_follows_the_setting_store"};
  const auto store = std::make_shared<arcstd::setting::setting_store>(folder.path());
  const auto size = store->create_setting("ui.size", std::int32_t{3});
  auto model = SettingsListModel{store};
  auto changedValue = 0;
  QObject::connect(
    &model,
    &SettingsListModel::dataChanged,
    [&](const QModelIndex&, const QModelIndex&, const QList<int>& roles)
    { changedValue += roles.contains(SettingsListModel::ValueRole) ? 1 : 0; }
  );

  // A value changed elsewhere, e.g. by a script
  CHECK(size->value(4));
  CHECK(test_utils::processEventsUntil([&] { return changedValue == 1; }));
  CHECK(roleData(model, 0, SettingsListModel::ValueRole) == QVariant{4});

  // A setting created later, an internal one is left out
  std::ignore = store->create_setting("internal.window.x", std::int32_t{0});
  std::ignore = store->type_erased_setting("ui.name", std::string{"name"});
  CHECK(test_utils::processEventsUntil([&] { return model.rowCount() == 2; }));
  CHECK(paths(model) == QStringList{"ui.size", "ui.name"});
}

} // namespace arcqml
