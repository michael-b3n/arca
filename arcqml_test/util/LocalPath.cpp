#include "test_utils/qt_strings.hpp"

#include "arcqml/util/LocalPath.hpp"

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QString>
#include <QUrl>

#include <filesystem>
#include <vector>

namespace arcqml
{
namespace
{

///
/// \return a local file of the tests, it need not exist
///
[[nodiscard]] auto localFile() -> std::filesystem::path
{
  return std::filesystem::temp_directory_path() / "arcqml_test" / "a file.txt";
}

} // namespace

TEST_CASE("LocalPath_converts_between_paths_and_urls", "[util]")
{
  const auto localPath = LocalPath{};
  const auto path = QString::fromStdWString(localFile().generic_wstring());
  const auto url = localPath.toUrl(path);
  CHECK(url.isLocalFile());
  CHECK(url.scheme() == QStringLiteral("file"));
  CHECK(localPath.fromUrl(url) == path);
  // Nothing on this machine
  CHECK(localPath.toUrl(QString{}).isEmpty());
  CHECK(localPath.fromUrl(QUrl{QStringLiteral("https://example.com/a.txt")}).isEmpty());
}

TEST_CASE("LocalPath_takes_the_local_files_a_file_dialog_reports", "[util]")
{
  const auto urls = QList<QUrl>{
    QUrl::fromLocalFile(QString::fromStdWString(localFile().generic_wstring())),
    QUrl{QStringLiteral("https://example.com/a.txt")},
  };
  CHECK(LocalPath::toLocalPaths(urls) == std::vector{localFile()});
  CHECK(LocalPath::toLocalPaths({}).empty());
}

} // namespace arcqml
