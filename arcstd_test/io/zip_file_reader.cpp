#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/io/zip_file_reader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

// #embed is standard in C++26, clang still calls it an extension
#ifdef __clang__
  #pragma clang diagnostic ignored "-Wc23-extensions"
#endif

namespace arcstd::io
{
namespace
{

// Written by Python's zipfile: "readme.txt" stored with the comment "entry comment", the folder
// "folder/" and "folder/Data.txt" deflated, with the archive comment "archive comment"
constexpr auto archive_bytes = std::to_array<unsigned char>({
#embed "../res/archive.zip"
});

constexpr auto readme_text = std::string_view{"hello zip\n"};

///
/// \return the test archive
///
[[nodiscard]] auto archive() -> std::span<const std::byte>
{
  return std::as_bytes(std::span{archive_bytes});
}

///
/// \return the text of the deflated entry
///
[[nodiscard]] auto data_text() -> std::string
{
  return std::views::repeat(std::string_view{"data "}, 50) | std::views::join |
         std::ranges::to<std::string>();
}

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

///
/// \return the index of the entry \p name of \p reader, std::nullopt if it has none
///
[[nodiscard]] auto index_of(
  const zip_file_reader& reader,
  const std::string& name,
  const zip_file_reader::query_flags_type flags = {}
) -> std::optional<std::uint64_t>
{
  return reader.entry(name, flags).and_then([](const auto& entry) { return entry.index; });
}

///
/// Check that \p reader is not open and has nothing to read.
///
auto check_not_open(const zip_file_reader& reader) -> void
{
  CHECK_FALSE(reader.is_open());
  CHECK(reader.entry_count() == 0);
  CHECK(reader.comment().empty());
  CHECK(reader.entries().empty());
  CHECK(reader.entry(0) == std::nullopt);
  CHECK(reader.entry("readme.txt") == std::nullopt);
  CHECK(reader.read_entry({.index = 0, .size = 1}).empty());
}

} // namespace

TEST_CASE("zip_file_reader_opens_an_archive_in_memory", "[io]")
{
  const auto data = empty_archive();
  const auto reader = zip_file_reader{std::span{data}};
  CHECK(reader.is_open());
  CHECK(reader.entry_count() == 0);
  CHECK(reader.comment().empty());
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

TEST_CASE("zip_file_reader_describes_the_entries", "[io]")
{
  using compression_method = zip_file_reader::compression_method;
  using encryption_method = zip_file_reader::encryption_method;

  const auto reader = zip_file_reader{archive()};
  REQUIRE(reader.is_open());
  CHECK(reader.entry_count() == 3);
  CHECK(reader.comment() == "archive comment");

  const auto entries = reader.entries();
  REQUIRE(entries.size() == 3);
  CHECK(
    (entries | std::views::transform(&zip_file_reader::zip_entry::name) |
     std::ranges::to<std::vector>()) ==
    std::vector<std::optional<std::string>>{"readme.txt", "folder/", "folder/Data.txt"}
  );

  const auto& readme = entries.at(0);
  CHECK(readme.comment == "entry comment");
  CHECK(readme.index == 0);
  CHECK(readme.compression == compression_method::store);
  CHECK(readme.encryption == encryption_method::none);
  CHECK(readme.size == readme_text.size());
  CHECK(readme.compressed_size == readme_text.size());
  CHECK(readme.crc == 0x4811c948);

  const auto& data = entries.at(2);
  CHECK(data.comment.empty());
  CHECK(data.compression == compression_method::deflate);
  CHECK(data.size == data_text().size());
  CHECK(data.compressed_size < data.size);
}

TEST_CASE("zip_file_reader_finds_an_entry", "[io]")
{
  using query_flag = zip_file_reader::query_flag;

  const auto reader = zip_file_reader{archive()};
  CHECK(reader.entry(1).and_then([](const auto& entry) { return entry.name; }) == "folder/");
  CHECK(reader.entry(3) == std::nullopt);

  CHECK(index_of(reader, "readme.txt") == 0);
  CHECK(index_of(reader, "missing.txt") == std::nullopt);
  CHECK(index_of(reader, "folder/data.txt") == std::nullopt);
  CHECK(index_of(reader, "folder/data.txt", query_flag::case_insensitive) == 2);
  CHECK(index_of(reader, "Data.txt") == std::nullopt);
  CHECK(index_of(reader, "Data.txt", query_flag::exclude_directories) == 2);
}

TEST_CASE("zip_file_reader_reads_the_content_of_an_entry", "[io]")
{
  const auto reader = zip_file_reader{archive()};
  const auto readme = reader.entry("readme.txt");
  REQUIRE(readme);
  CHECK(reader.read_entry_as_string(*readme) == readme_text);
  CHECK(reader.read_entry(*readme).size() == readme_text.size());

  const auto data = reader.entry("folder/Data.txt");
  REQUIRE(data);
  CHECK(reader.read_entry_as_string(*data) == data_text());

  // A folder has no content, an entry of no index or size names none
  CHECK(reader.read_entry_as_string(reader.entry("folder/").value()).empty());
  CHECK(reader.read_entry({.size = 1}).empty());
  CHECK(reader.read_entry({.index = 0}).empty());
}

TEST_CASE("zip_file_reader_opens_an_archive_file", "[io]")
{
  const auto folder = test_utils::temp_folder{"zip_file_reader_opens_an_archive_file"};
  const auto file = folder.path() / "archive.zip";
  test_utils::write_file(file, std::string{std::from_range, archive_bytes});

  const auto reader = zip_file_reader{file};
  CHECK(reader.entry_count() == 3);
  CHECK(reader.read_entry_as_string(reader.entry(0).value()) == readme_text);
}

TEST_CASE("zip_file_reader_is_not_open_without_an_archive", "[io]")
{
  const auto folder = test_utils::temp_folder{"zip_file_reader_is_not_open_without_an_archive"};
  check_not_open(zip_file_reader{folder.path() / "missing.zip"});

  const auto text = folder.path() / "text.zip";
  test_utils::write_file(text, "no archive");
  check_not_open(zip_file_reader{text});

  const auto bytes = std::array<std::byte, 4>{std::byte{'P'}, std::byte{'K'}};
  check_not_open(zip_file_reader{std::span{bytes}});
}

} // namespace arcstd::io
