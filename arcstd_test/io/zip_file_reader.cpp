#include <arcstd/io/zip_file_reader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <utility>

namespace arcstd::io
{
namespace
{

///
/// \return an archive without entries: its end of central directory record only
///
[[nodiscard]] constexpr auto empty_archive() -> std::array<std::byte, 22>
{
  auto bytes = std::array<std::byte, 22>{};
  bytes[0] = std::byte{'P'};
  bytes[1] = std::byte{'K'};
  bytes[2] = std::byte{0x05};
  bytes[3] = std::byte{0x06};
  return bytes;
}

} // namespace

TEST_CASE("zip_file_reader_opens_an_archive_in_memory", "[io]")
{
  const auto data = empty_archive();
  const auto reader = zip_file_reader{std::span{data}};
  CHECK(reader.is_open());
  CHECK(reader.entry_count() == 0);
}

TEST_CASE("zip_file_reader_moves_the_archive", "[io]")
{
  const auto data = empty_archive();
  auto reader = zip_file_reader{std::span{data}};
  const auto moved = std::move(reader);
  CHECK(moved.is_open());
  // NOLINTNEXTLINE(bugprone-use-after-move) the moved-from reader holds the archive no more
  CHECK_FALSE(reader.is_open());

  auto assigned = zip_file_reader{std::span{data}};
  assigned = zip_file_reader{std::span{data}};
  CHECK(assigned.is_open());
}

} // namespace arcstd::io
