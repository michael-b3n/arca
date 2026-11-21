#include "qml_test_utils/qt_strings.hpp"
#include "test_utils/settings.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/util/SettingConversion.hpp"

#include <arcstd/setting/setting_store.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QColor>
#include <QJSEngine>
#include <QJSValue>
#include <QPointF>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace arcqml
{
namespace
{

// Typedefs
using DefaultValueType = arcstd::setting::setting_store::setting_type_erased_variant_type;

enum class mode
{
  light,
  dark,
};

} // namespace

TEST_CASE("SettingConversion_unwraps_a_javascript_value", "[util]")
{
  auto engine = QJSEngine{};
  const auto array = QVariant::fromValue(engine.evaluate(QStringLiteral("[1, 'a']")));
  CHECK(tryNormalizeQVariant(array).toList() == QVariantList{1, "a"});
  // Any other value stays as it is
  CHECK(tryNormalizeQVariant(QVariant{2.5}) == QVariant{2.5});
  CHECK_FALSE(tryNormalizeQVariant(QVariant{}).isValid());
}

TEST_CASE("SettingConversion_deduces_the_default_value_of_a_setting", "[util]")
{
  CHECK(toDefaultSettingValue(QVariant{true}) == std::optional<DefaultValueType>{true});
  CHECK(toDefaultSettingValue(QVariant{3}) == std::optional<DefaultValueType>{std::int64_t{3}});
  CHECK(toDefaultSettingValue(QVariant{2.5F}) == std::optional<DefaultValueType>{2.5});
  CHECK(
    toDefaultSettingValue(QVariant{QStringLiteral("text")}) ==
    std::optional<DefaultValueType>{std::string{"text"}}
  );
  CHECK(
    toDefaultSettingValue(QVariant::fromValue(QColor{0x12, 0x34, 0x56, 0x78})) ==
    std::optional<DefaultValueType>{std::string{"#78123456"}}
  );
  // A number QML declares as JavaScript value
  auto engine = QJSEngine{};
  CHECK(
    toDefaultSettingValue(QVariant::fromValue(engine.toScriptValue(4))) ==
    std::optional<DefaultValueType>{std::int64_t{4}}
  );
  // Values of no setting type
  CHECK(toDefaultSettingValue(QVariant::fromValue(QPointF{1.0, 2.0})) == std::nullopt);
  CHECK(toDefaultSettingValue(QVariant{}) == std::nullopt);
}

TEST_CASE("SettingConversion_reads_the_value_of_a_setting", "[util]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingConversion_reads_the_value_of_a_setting"};
  auto store = arcstd::setting::setting_store{folder.path()};
  CHECK(toQmlValue(store.type_erased_setting("test.flag", true)) == QVariant{true});
  CHECK(toQmlValue(store.type_erased_setting("test.count", std::uint32_t{7})) == QVariant{7});
  CHECK(toQmlValue(store.type_erased_setting("test.ratio", 0.5)) == QVariant{0.5});
  CHECK(
    toQmlValue(store.type_erased_setting("test.name", std::string{"name"})) ==
    QVariant{QStringLiteral("name")}
  );
  CHECK(
    toQmlValue(store.type_erased_setting("test.folder", std::filesystem::path{"a/b"})) ==
    QVariant{QStringLiteral("a/b")}
  );
  // A duration counts the period it is stored in
  CHECK(
    toQmlValue(store.type_erased_setting("test.pause", std::chrono::milliseconds{250})) ==
    QVariant{250}
  );
  const auto counts = std::vector<std::int64_t>{1, 2};
  CHECK(
    toQmlValue(store.type_erased_setting("test.counts", counts)) == QVariant{
                                                                      QVariantList{1, 2}
  }
  );
  CHECK(
    toQmlValue(store.type_erased_setting("test.some", std::optional<std::int32_t>{5})) ==
    QVariant{5}
  );
  // No value, or one that does not fit a QML number
  CHECK_FALSE(
    toQmlValue(store.type_erased_setting("test.none", std::optional<std::int32_t>{})).isValid()
  );
  CHECK_FALSE(
    toQmlValue(store.type_erased_setting("test.large", std::numeric_limits<std::int64_t>::max()))
      .isValid()
  );
}

TEST_CASE("SettingConversion_writes_the_value_of_a_setting", "[util]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingConversion_writes_the_value_of_a_setting"};
  auto store = arcstd::setting::setting_store{folder.path()};

  const auto flag = store.type_erased_setting("test.flag", false);
  CHECK(setQmlValue(flag, true));
  CHECK(arcstd::test_utils::type_erased_setting<bool>(flag)->value() == true);

  const auto count = store.type_erased_setting("test.count", std::int32_t{0});
  CHECK(setQmlValue(count, 7));
  CHECK(arcstd::test_utils::type_erased_setting<std::int32_t>(count)->value() == 7);

  const auto name = store.type_erased_setting("test.name", std::string{});
  CHECK(setQmlValue(name, QStringLiteral("name")));
  CHECK(arcstd::test_utils::type_erased_setting<std::string>(name)->value() == "name");

  const auto pause = store.type_erased_setting("test.pause", std::chrono::seconds{1});
  CHECK(setQmlValue(pause, 30));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::chrono::seconds>(pause)->value() ==
    std::chrono::seconds{30}
  );

  const auto some = store.type_erased_setting("test.some", std::optional<double>{});
  CHECK(setQmlValue(some, 1.5));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::optional<double>>(some)->value() ==
    std::optional{1.5}
  );

  // Lists, also as JavaScript arrays
  const auto counts = store.type_erased_setting("test.counts", std::vector<std::uint64_t>{});
  CHECK(setQmlValue(counts, QVariantList{1, 2}));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::vector<std::uint64_t>>(counts)->value() ==
    std::vector<std::uint64_t>{1, 2}
  );
  auto engine = QJSEngine{};
  const auto names = store.type_erased_setting("test.names", std::vector<std::string>{});
  CHECK(setQmlValue(names, QVariant::fromValue(engine.evaluate(QStringLiteral("['a', 'b']")))));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::vector<std::string>>(names)->value() ==
    std::vector<std::string>{"a", "b"}
  );
  const auto pauses = store.type_erased_setting("test.pauses", std::vector<std::chrono::minutes>{});
  CHECK(setQmlValue(pauses, QVariantList{2}));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::vector<std::chrono::minutes>>(pauses)->value() ==
    std::vector{std::chrono::minutes{2}}
  );
}

TEST_CASE("SettingConversion_rejects_a_value_the_setting_does_not_take", "[util]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingConversion_rejects_a_value_the_setting_does_not_take"};
  auto store = arcstd::setting::setting_store{folder.path()};

  // A negative number for an unsigned setting
  const auto count = store.type_erased_setting("test.count", std::uint32_t{3});
  CHECK_FALSE(setQmlValue(count, -1));
  CHECK(arcstd::test_utils::type_erased_setting<std::uint32_t>(count)->value() == 3);
  const auto counts = store.type_erased_setting("test.counts", std::vector<std::uint32_t>{3});
  CHECK_FALSE(setQmlValue(counts, QVariantList{1, -1}));
  CHECK(
    arcstd::test_utils::type_erased_setting<std::vector<std::uint32_t>>(counts)->value() ==
    std::vector<std::uint32_t>{3}
  );

  // The validator corrects a value out of its range
  const auto range =
    std::make_shared<arcstd::setting::setting_validator_range<std::int32_t>>(0, 10);
  const auto size = store.type_erased_setting("test.size", std::int32_t{3}, range);
  CHECK_FALSE(setQmlValue(size, 20));
  CHECK(arcstd::test_utils::type_erased_setting<std::int32_t>(size)->value() == 9);

  // An enum takes the names of its values only
  std::ignore = store.create_setting("test.mode", mode::light);
  const auto modeSetting = store.type_erased_setting("test.mode");
  REQUIRE(modeSetting);
  CHECK(setQmlValue(*modeSetting, QStringLiteral("dark")));
  CHECK_THROWS_AS(setQmlValue(*modeSetting, QStringLiteral("purple")), arcstd::util::exception);
  CHECK(arcstd::test_utils::type_erased_setting<std::string>(*modeSetting)->value() == "dark");
}

TEST_CASE("SettingConversion_names_the_postfix_of_a_setting", "[util]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingConversion_names_the_postfix_of_a_setting"};
  auto store = arcstd::setting::setting_store{folder.path()};
  CHECK(toQmlPostfix(store.type_erased_setting("test.size", std::int32_t{1}, "px")) == "px");
  CHECK(toQmlPostfix(store.type_erased_setting("test.count", std::int32_t{1})).isEmpty());
  // A duration without a postfix of its own takes the unit of its period
  CHECK(toQmlPostfix(store.type_erased_setting("test.ms", std::chrono::milliseconds{1})) == "ms");
  CHECK(
    toQmlPostfix(store.type_erased_setting("test.s", std::optional<std::chrono::seconds>{})) == "s"
  );
  CHECK(
    toQmlPostfix(store.type_erased_setting("test.min", std::vector<std::chrono::minutes>{})) ==
    "min"
  );
  CHECK(
    toQmlPostfix(store.type_erased_setting("test.wait", std::chrono::seconds{1}, "sec")) == "sec"
  );
}

TEST_CASE("SettingConversion_lists_the_values_of_a_list_validator", "[util]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"SettingConversion_lists_the_values_of_a_list_validator"};
  auto store = arcstd::setting::setting_store{folder.path()};
  const auto list = std::make_shared<arcstd::setting::setting_validator_list<std::int32_t>>(
    std::vector<std::int32_t>{1, 2, 4}
  );
  CHECK(
    toQmlValidatorValues(store.type_erased_setting("test.list", std::int32_t{1}, list)) ==
    QVariantList{1, 2, 4}
  );
  std::ignore = store.create_setting("test.mode", mode::light);
  const auto modeSetting = store.type_erased_setting("test.mode");
  REQUIRE(modeSetting);
  CHECK(toQmlValidatorValues(*modeSetting) == QVariantList{"light", "dark"});
  // No list validator, no values
  const auto range = std::make_shared<arcstd::setting::setting_validator_range<double>>(0.0, 1.0);
  CHECK(toQmlValidatorValues(store.type_erased_setting("test.range", 0.5, range)).isEmpty());
  CHECK(toQmlValidatorValues(store.type_erased_setting("test.free", 0.5)).isEmpty());
}

} // namespace arcqml
