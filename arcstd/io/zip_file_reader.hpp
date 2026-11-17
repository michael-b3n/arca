#pragma once

#include "arcstd/util/bitflags.hpp"
#include "arcstd/util/non_owning_ptr.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct zip;
struct zip_stat;

namespace arcstd::io
{

///
/// Reads the entries of a ZIP archive.
///
class zip_file_reader final
{
  // Typedefs
  struct archive_closer final
  {
    auto operator()(util::non_owning_ptr<zip> archive) const noexcept -> void;
  };
  using archive_ptr = std::unique_ptr<zip, archive_closer>;

  // Variables
  std::string password_;
  archive_ptr zip_handle_;

public: // Typedefs
  enum class compression_method
  {
    default_method,
    store, // no compression
    deflate,
    bzip2,
    lzma,
    zstd,
    xz,
  };

  enum class encryption_method
  {
    none,
    trad_pkware,
    aes_128,
    aes_192,
    aes_256,
  };

  ///
  /// How an entry is looked up by name.
  ///
  enum class query_flag
  {
    exclude_directories,
    case_insensitive,
  };
  using query_flags_type = util::bitflags<query_flag>;

  ///
  /// File or folder of the archive, a field is std::nullopt if libzip does not know it.
  ///
  struct zip_entry final
  {
    std::string comment;
    std::optional<std::string> name; // its path in the archive
    std::optional<std::uint64_t> index;
    std::optional<compression_method> compression;
    std::optional<encryption_method> encryption;
    std::optional<std::uint64_t> size; // uncompressed
    std::optional<std::uint64_t> compressed_size;
    std::optional<std::uint32_t> crc;
  };

public: // Structors
  explicit zip_file_reader(const std::filesystem::path& zip_path, std::string_view password = "");
  explicit zip_file_reader(std::span<const std::byte> data, std::string_view password = "");
  ~zip_file_reader() noexcept;
  zip_file_reader(const zip_file_reader&) = delete;
  zip_file_reader(zip_file_reader&&) noexcept;

public: // Operators
  auto operator=(const zip_file_reader&) -> zip_file_reader& = delete;
  auto operator=(zip_file_reader&&) noexcept -> zip_file_reader&;

public: // Accessors
  ///
  /// \return false if the archive could not be opened, it then has no entries
  ///
  [[nodiscard]] auto is_open() const -> bool;
  [[nodiscard]] auto entry_count() const -> std::size_t;

  ///
  /// \return the comment of the archive, an empty one if it has none or is not open
  ///
  [[nodiscard]] auto comment() const -> std::string;
  [[nodiscard]] auto entries() const -> std::vector<zip_entry>;

  ///
  /// \return the entry \p index, std::nullopt if there is none
  ///
  [[nodiscard]] auto entry(std::size_t index) const -> std::optional<zip_entry>;

  ///
  /// \return the entry \p name, std::nullopt if there is none
  ///
  [[nodiscard]] auto entry(const std::string& name, query_flags_type flags = {}) const
    -> std::optional<zip_entry>;

  ///
  /// \return the content of \p entry, empty if it can not be read
  ///
  [[nodiscard]] auto read_entry(const zip_entry& entry) const -> std::vector<std::byte>;

  ///
  /// \return the content of \p entry as text, empty if it can not be read
  ///
  [[nodiscard]] auto read_entry_as_string(const zip_entry& entry) const -> std::string;

private: // Implementation
  ///
  /// \return the index of the entry \p name, std::nullopt if there is none
  ///
  [[nodiscard]] auto index_of_entry(const std::string& name, query_flags_type flag = {}) const
    -> std::optional<std::size_t>;

  ///
  /// \return the entry \p index as libzip reads it with \p flag, std::nullopt if there is none
  ///
  [[nodiscard]] auto entry_by_index(std::size_t index, std::uint32_t flag) const
    -> std::optional<zip_entry>;

  ///
  /// \return the entry libzip describes by \p stat
  ///
  [[nodiscard]] auto create_entry(const zip_stat& stat) const -> zip_entry;

  ///
  /// \return the last error of libzip
  ///
  [[nodiscard]] auto error_message() const -> std::string;
};

} // namespace arcstd::io
