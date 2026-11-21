#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/setting/property.hpp>
#include <arcstd/setting/property_node.hpp>
#include <arcstd/setting/property_tree.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

namespace arcstd::setting
{
namespace
{

///
/// \return the text of \p file without carriage returns, the tree writes it in text mode
///
[[nodiscard]] auto read_text(const std::filesystem::path& file) -> std::string
{
  auto text = test_utils::read_file(file);
  std::erase(text, '\r');
  return text;
}

///
/// \return the value of the setting "name" in \p file, which keeps it from then on
///
[[nodiscard]] auto load_and_store(const std::filesystem::path& file) -> std::string
{
  const auto tree = property_tree::create(file);
  const auto value = property<std::string>{
    property_node{tree, "settings.name"},
    std::string{"default"}
  };
  return value.value();
}

} // namespace

TEST_CASE("property_tree_drops_the_whitespace_between_elements", "[setting]")
{
  const auto folder =
    test_utils::temp_folder{"property_tree_drops_the_whitespace_between_elements"};
  const auto file = folder.path() / "settings.xml";
  // Whitespace an earlier version piled up between the elements on every write
  test_utils::write_file(
    file,
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<settings>\n\n\n  \n  \n<name>\n\n  "
    "stored\n</name></settings>\n"
  );

  CHECK(load_and_store(file) == "stored");
  const auto first = read_text(file);
  CHECK_FALSE(first.contains("\n\n"));
  CHECK(first.contains("<name>stored</name>"));

  // A file that was written once stays as it is
  CHECK(load_and_store(file) == "stored");
  CHECK(read_text(file) == first);
}

TEST_CASE("property_tree_is_one_per_file", "[setting]")
{
  const auto folder = test_utils::temp_folder{"property_tree_is_one_per_file"};
  const auto file = folder.path() / "settings.xml";
  const auto tree = property_tree::create(file);
  CHECK(property_tree::create(file) == tree);
  CHECK(property_tree::create(folder.path() / "other.xml") != tree);
  CHECK_THROWS_AS(property_tree{file}, util::exception);
  CHECK_THROWS_AS(property_tree{folder.path() / "settings.txt"}, util::exception);
}

TEST_CASE("property_tree_starts_anew_from_a_broken_file", "[setting]")
{
  const auto folder = test_utils::temp_folder{"property_tree_starts_anew_from_a_broken_file"};
  const auto file = folder.path() / "settings.xml";
  test_utils::write_file(file, "<settings><name>stored");
  CHECK(load_and_store(file) == "default");
  CHECK(read_text(file).contains("<name>default</name>"));
}

} // namespace arcstd::setting
