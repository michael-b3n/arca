#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include "arcqml/bridge/BridgeLogger.hpp"

#include <arcstd/util/log.hpp>

#include <catch2/catch_test_macros.hpp>

#include <QString>

#include <string>

namespace arcqml
{

TEST_CASE("BridgeLogger_logs_through_the_log_of_arcstd", "[bridge]")
{
  const auto folder =
    arcstd::test_utils::temp_folder{"BridgeLogger_logs_through_the_log_of_arcstd"};
  {
    const auto logger = arcstd::util::logger{folder.path()};
    const auto qmlLogger = BridgeLogger{};
    qmlLogger.debug(QStringLiteral("debug 1"));
    qmlLogger.info(QStringLiteral("info 2"));
    qmlLogger.warning(QStringLiteral("warning 3"));
    qmlLogger.error(QStringLiteral("error 4"));
  }
  const auto latest = arcstd::test_utils::read_file(folder.path() / "logs" / "latest.log");
  CHECK(latest.contains("QML: info 2"));
  CHECK(latest.contains("QML: warning 3"));
  CHECK(latest.contains("QML: error 4"));
  const auto logsDebug = arcstd::util::global_log_level() == arcstd::util::logger_level::debug;
  CHECK(latest.contains("QML: debug 1") == logsDebug);
}

} // namespace arcqml
