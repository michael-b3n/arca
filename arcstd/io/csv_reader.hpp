#pragma once

#include "arcstd/export.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rapidcsv
{
class Document;
} // namespace rapidcsv

namespace arcstd::io
{

///
/// Reads the cells of a CSV document as strings, the whole document is held in memory.
///
class ARCSTD_IO_EXPORT csv_reader final
{
  // Variables
  std::unique_ptr<rapidcsv::Document> document_;

public: // Typedefs
  ///
  /// The layout of the document.
  ///
  struct params final
  {
    char separator{','};
    bool header{true}; // the first row names the columns
    bool trim{false};  // of the whitespace around a cell
    bool skip_comment_lines{false};
    bool skip_empty_lines{false};
  };

public: // Constants
  static constexpr char comment_prefix = '#';

public: // Structors
  ///
  /// Reader of the document \p data, e.g. a compiled in resource.
  /// \throws util::exception if the data cannot be parsed
  ///
  csv_reader(std::span<const std::byte> data, const params& p);

  ///
  /// Reader of the document file \p csv_path.
  /// \throws util::exception if the file cannot be read or parsed
  ///
  csv_reader(const std::filesystem::path& csv_path, const params& p);

  ~csv_reader() noexcept;
  csv_reader(const csv_reader&) = delete;
  csv_reader(csv_reader&&) noexcept;

public: // Operators
  auto operator=(const csv_reader&) -> csv_reader& = delete;
  auto operator=(csv_reader&&) noexcept -> csv_reader&;

public: // Accessors
  ///
  /// \return the number of rows, without the header row
  ///
  [[nodiscard]] auto row_count() const -> std::size_t;
  [[nodiscard]] auto column_count() const -> std::size_t;

  ///
  /// \return the names of the header row, none without one
  ///
  [[nodiscard]] auto column_names() const -> std::vector<std::string>;

  ///
  /// \return the index of the column \p name, std::nullopt if there is none
  ///
  [[nodiscard]] auto column_index(std::string_view name) const -> std::optional<std::size_t>;

  ///
  /// \return the cell of \p column and \p row, std::nullopt if there is none
  ///
  [[nodiscard]] auto cell(std::size_t column, std::size_t row) const -> std::optional<std::string>;
  [[nodiscard]] auto cell(std::string_view column, std::size_t row) const
    -> std::optional<std::string>;

  ///
  /// \return the cells of \p row, none if there is no such row
  ///
  [[nodiscard]] auto row(std::size_t row) const -> std::vector<std::string>;

  ///
  /// \return the cells of \p column, none if there is no such column
  ///
  [[nodiscard]] auto column(std::string_view column) const -> std::vector<std::string>;
};

} // namespace arcstd::io
