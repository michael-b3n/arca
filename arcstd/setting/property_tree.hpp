#pragma once

#ifndef ARCSTD_INTERNAL
  #error "private header of arcstd, a project includes the public ones only"
#endif

#include "arcstd/setting/property_parser.hpp"

#include <filesystem>
#include <memory>
#include <mutex>

namespace arcstd::setting
{

///
/// Property tree. This class loads the settings file and saves it again, its nodes hold the properties, \see property_node.
/// \note This class must be created as a shared pointer.
///
class property_tree final : public std::enable_shared_from_this<property_tree>
{
  // Friends
  friend class property_node;

  // Variables
  inline static std::mutex trees_mtx_{};
  inline static std::vector<std::weak_ptr<property_tree>> trees_{};
  mutable std::mutex mtx_;
  std::filesystem::path tree_file_path_;
  property_tree_type tree_;

public: // Typedefs
  using sptr_type = std::shared_ptr<property_tree>;
  using path_type = property_path_type;

public: // Creator
  ///
  /// Property tree creator
  /// \return
  ///
  [[nodiscard]] static auto create(const std::filesystem::path& tree_file_path) -> property_tree::sptr_type;

public: // Structors
  property_tree() = default;
  explicit property_tree(const std::filesystem::path& tree_file_path);
  ~property_tree() noexcept;
};

} // namespace arcstd::setting
