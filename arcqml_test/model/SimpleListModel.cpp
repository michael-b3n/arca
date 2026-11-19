#include "test_utils/qt_strings.hpp"

#include "arcqml/model/SimpleListModel.hpp"

#include <catch2/catch_test_macros.hpp>

#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <QVariant>
#include <QVariantList>

namespace arcqml
{
namespace
{

///
/// \return the values of \p model as text, in the order of its rows
///
[[nodiscard]] auto texts(const SimpleListModel& model) -> QStringList
{
  auto result = QStringList{};
  for(auto row = 0; row < model.rowCount(); ++row)
  {
    result.append(model.data(model.index(row), SimpleListModel::ValueRole).toString());
  }
  return result;
}

} // namespace

TEST_CASE("SimpleListModel_adds_and_removes_entries", "[model]")
{
  auto model = SimpleListModel{};
  CHECK(model.rowCount() == 0);
  model.append(QStringLiteral("b"));
  model.append(QStringLiteral("c"));
  model.prepend(QStringLiteral("a"));
  CHECK(texts(model) == QStringList{"a", "b", "c"});
  CHECK(model.entries() == QVariantList{"a", "b", "c"});
  CHECK(model.roleNames().value(SimpleListModel::ValueRole) == "value");
  // A list has no children
  CHECK(model.rowCount(model.index(0)) == 0);

  CHECK(model.remove(model.index(1)));
  CHECK(texts(model) == QStringList{"a", "c"});
  CHECK_FALSE(model.remove(model.index(2)));
  CHECK_FALSE(model.remove(QModelIndex{}));

  model.clear();
  CHECK(model.rowCount() == 0);
}

TEST_CASE("SimpleListModel_sets_the_value_role_only", "[model]")
{
  auto model = SimpleListModel{};
  model.append(1);
  auto changed = 0;
  QObject::connect(&model, &SimpleListModel::dataChanged, [&] { ++changed; });

  CHECK(model.setData(model.index(0), 2, SimpleListModel::ValueRole));
  CHECK(model.data(model.index(0), SimpleListModel::ValueRole) == QVariant{2});
  CHECK(changed == 1);
  CHECK_FALSE(model.setData(model.index(0), 3, Qt::DisplayRole));
  CHECK_FALSE(model.setData(model.index(1), 3, SimpleListModel::ValueRole));
  CHECK(model.data(model.index(0), SimpleListModel::ValueRole) == QVariant{2});
  CHECK(changed == 1);
  // Other roles and rows hold nothing
  CHECK_FALSE(model.data(model.index(0), Qt::DisplayRole).isValid());
  CHECK_FALSE(model.data(model.index(1), SimpleListModel::ValueRole).isValid());
  CHECK(model.flags(model.index(0)) == Qt::NoItemFlags);
}

TEST_CASE("SimpleListModel_moves_an_entry_up_and_down", "[model]")
{
  auto model = SimpleListModel{};
  CHECK(model.replace(QVariantList{"a", "b", "c", "d"}));

  CHECK(model.move(model.index(0), model.index(2)));
  CHECK(texts(model) == QStringList{"b", "c", "a", "d"});
  CHECK(model.move(model.index(3), model.index(0)));
  CHECK(texts(model) == QStringList{"d", "b", "c", "a"});
  // Onto itself or out of the rows it does not move
  CHECK_FALSE(model.move(model.index(1), model.index(1)));
  CHECK_FALSE(model.move(model.index(1), model.index(4)));
  CHECK(texts(model) == QStringList{"d", "b", "c", "a"});
}

TEST_CASE("SimpleListModel_replaces_its_entries_with_a_list", "[model]")
{
  auto model = SimpleListModel{};
  model.append(QStringLiteral("old"));
  CHECK(model.replace(QVariantList{1, 2}));
  CHECK(model.entries() == QVariantList{1, 2});

  // A JavaScript array as QML passes it
  auto engine = QJSEngine{};
  CHECK(model.replace(QVariant::fromValue(engine.evaluate(QStringLiteral("['x', 'y']")))));
  CHECK(texts(model) == QStringList{"x", "y"});

  // A value of no list leaves the entries
  CHECK_FALSE(model.replace(QVariant::fromValue(QModelIndex{})));
  CHECK(texts(model) == QStringList{"x", "y"});
}

TEST_CASE("SimpleListModel_finds_a_value_by_its_text", "[model]")
{
  auto model = SimpleListModel{};
  CHECK(model.replace(QVariantList{"1", 2, QVariant{}}));
  CHECK(model.indexOfValue(1) == 0);
  CHECK(model.indexOfValue(QStringLiteral("2")) == 1);
  // An empty value is found by an empty text
  CHECK(model.indexOfValue(QString{}) == 2);
  CHECK(model.indexOfValue(3) == -1);
}

} // namespace arcqml
