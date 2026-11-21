#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/io/csv_reader.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace arcstd::io
{
namespace
{

using strings = std::vector<std::string>;

///
/// \return the reader of \p text
///
[[nodiscard]] auto read_text(const std::string_view text, const csv_reader::params& p = {})
  -> csv_reader
{
  return csv_reader{std::as_bytes(std::span{text}), p};
}

} // namespace

TEST_CASE("csv_reader_reads_the_cells_below_the_header", "[io]")
{
  const auto reader = read_text("name,age\nanna,30\nben,41\n");
  CHECK(reader.row_count() == 2);
  CHECK(reader.column_count() == 2);
  CHECK(reader.column_names() == strings{"name", "age"});
  CHECK(reader.column_index("age") == 1);
  CHECK(reader.column_index("size") == std::nullopt);
  CHECK(reader.cell(0, 1) == "ben");
  CHECK(reader.cell("age", 0) == "30");
  CHECK(reader.row(1) == strings{"ben", "41"});
  CHECK(reader.column("name") == strings{"anna", "ben"});
}

TEST_CASE("csv_reader_has_no_cell_outside_the_document", "[io]")
{
  const auto reader = read_text("name,age\nanna,30\n");
  CHECK(reader.cell(2, 0) == std::nullopt);
  CHECK(reader.cell(0, 1) == std::nullopt);
  CHECK(reader.cell("size", 0) == std::nullopt);
  CHECK(reader.row(1).empty());
  CHECK(reader.column("size").empty());
}

TEST_CASE("csv_reader_fills_a_short_row_with_empty_cells", "[io]")
{
  const auto reader = read_text("name,age\nanna\n");
  CHECK(reader.cell(1, 0) == std::nullopt);
  CHECK(reader.row(0) == strings{"anna", ""});
  CHECK(reader.column("age") == strings{""});
}

TEST_CASE("csv_reader_reads_a_document_without_header", "[io]")
{
  const auto reader = read_text("anna,30\nben,41\n", {.header = false});
  CHECK(reader.row_count() == 2);
  CHECK(reader.column_names().empty());
  CHECK(reader.cell(0, 0) == "anna");
  CHECK(reader.row(1) == strings{"ben", "41"});
}

TEST_CASE("csv_reader_takes_the_layout_of_its_params", "[io]")
{
  const auto text = std::string_view{"name; age\n# comment\n anna ;30\n\nben; 41\n"};
  const auto reader = read_text(
    text, {.separator = ';', .trim = true, .skip_comment_lines = true, .skip_empty_lines = true}
  );
  CHECK(reader.column_names() == strings{"name", "age"});
  CHECK(reader.row_count() == 2);
  CHECK(reader.row(0) == strings{"anna", "30"});
  CHECK(reader.row(1) == strings{"ben", "41"});

  // Without trim and skipping, the whitespace and the comment are cells
  const auto plain = read_text(text, {.separator = ';'});
  CHECK(plain.column_names() == strings{"name", " age"});
  CHECK(plain.cell(0, 0) == "# comment");
}

TEST_CASE("csv_reader_reads_a_file", "[io]")
{
  const auto folder = test_utils::temp_folder{"csv_reader_reads_a_file"};
  const auto file = folder.path() / "table.csv";
  test_utils::write_file(file, "name,age\nanna,30\n");
  const auto reader = csv_reader{file, {}};
  CHECK(reader.cell("name", 0) == "anna");

  CHECK_THROWS_AS((csv_reader{folder.path() / "missing.csv", {}}), util::exception);
}

TEST_CASE("csv_reader_moves_the_document", "[io]")
{
  auto reader = read_text("name\nanna\n");
  const auto moved = std::move(reader);
  CHECK(moved.cell(0, 0) == "anna");

  auto assigned = read_text("name\nben\n");
  assigned = read_text("name\ncarl\n");
  CHECK(assigned.cell(0, 0) == "carl");
}

} // namespace arcstd::io
