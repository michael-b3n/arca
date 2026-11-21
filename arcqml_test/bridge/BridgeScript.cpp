#include "test_utils/events.hpp"
#include "test_utils/files.hpp"
#include "test_utils/qt_strings.hpp"
#include "test_utils/scripts.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/bridge/BridgeScript.hpp"

#include <arcstd/lua/lua_engine.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <filesystem>
#include <format>
#include <memory>
#include <ranges>
#include <string>

namespace arcqml
{
namespace
{

///
/// \return Lua of the user script \p name, it offers the function "test.say"
///
[[nodiscard]] auto script(const std::string& name) -> std::string
{
  return std::format(
    "return {{name = '{0}', description = 'about {0}', node = '{0}', "
    "['test.say'] = function() return {{}} end}}",
    name
  );
}

///
/// \return the names of the scripts \p bridge lists
///
[[nodiscard]] auto scriptNames(const BridgeScript& bridge) -> QStringList
{
  return bridge.property("scripts").toList() |
         std::views::transform([](const QVariant& entry)
                               { return entry.toMap().value(QStringLiteral("name")).toString(); }) |
         std::ranges::to<QStringList>();
}

///
/// \return true if \p bridge is loading the scripts
///
[[nodiscard]] auto loading(const BridgeScript& bridge) -> bool
{
  return bridge.property("loading").toBool();
}

} // namespace

TEST_CASE("BridgeScript_lists_the_scripts_it_loads", "[bridge]")
{
  const auto folder = arcstd::test_utils::temp_folder{"BridgeScript_lists_the_scripts_it_loads"};
  const auto scripts = folder.path() / "scripts";
  std::filesystem::create_directories(scripts);
  arcstd::test_utils::write_file(scripts / "a.lua", script("a"));
  const auto engine = std::make_shared<arcstd::lua::lua_engine>(
    arcstd::test_utils::make_script_settings(folder.path(), true), arcstd::test_utils::no_bundled
  );
  auto bridge = BridgeScript{engine};
  auto loadingChanged = 0;
  QObject::connect(&bridge, &BridgeScript::loadingChanged, [&] { ++loadingChanged; });
  CHECK_FALSE(loading(bridge));
  CHECK(scriptNames(bridge).isEmpty());

  bridge.loadScripts();
  CHECK(loading(bridge));
  // A call made while the scripts load is ignored
  bridge.loadScripts();
  CHECK(loadingChanged == 1);
  CHECK(test_utils::processEventsUntil([&] { return !loading(bridge); }));
  CHECK(loadingChanged == 2);
  REQUIRE(scriptNames(bridge) == QStringList{"a"});
  const auto entry = bridge.property("scripts").toList().first().toMap();
  CHECK(entry.value(QStringLiteral("description")) == QStringLiteral("about a"));
  CHECK(entry.value(QStringLiteral("functions")).toStringList() == QStringList{"test.say"});
  bridge.disconnect();
}

TEST_CASE("BridgeScript_imports_the_files_a_file_dialog_reports", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"BridgeScript_imports_the_files_a_file_dialog_reports"};
  const auto source = folder.path() / "source";
  std::filesystem::create_directories(source);
  arcstd::test_utils::write_file(source / "b.lua", script("b"));
  const auto engine = std::make_shared<arcstd::lua::lua_engine>(
    arcstd::test_utils::make_script_settings(folder.path(), true), arcstd::test_utils::no_bundled
  );
  auto bridge = BridgeScript{engine};

  bridge.importFiles(
    {QUrl::fromLocalFile(QString::fromStdWString((source / "b.lua").wstring())),
     QUrl{QStringLiteral("https://example.com/c.lua")}}
  );
  CHECK(loading(bridge));
  CHECK(test_utils::processEventsUntil([&] { return !loading(bridge); }));
  CHECK(scriptNames(bridge) == QStringList{"b"});
  CHECK(std::filesystem::exists(engine->settings().folder / "b.lua"));
  bridge.disconnect();
}

} // namespace arcqml
