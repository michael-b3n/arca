#include <arcstd/setting/property_parser.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace arcstd::setting
{
namespace
{

enum class shape
{
  circle,
  square,
};

} // namespace

TEST_CASE("property_parser_reads_and_writes_composed_values", "[setting]")
{
  using pair_type = std::pair<std::int8_t, shape>;
  auto tree = property_tree_type{};
  property_parser::write(property_path_type{"shape"}, tree, shape::square);
  property_parser::write(property_path_type{"pair"}, tree, pair_type{-3, shape::circle});
  property_parser::write(property_path_type{"float"}, tree, 0.5F);
  CHECK(property_parser::read<shape>(property_path_type{"shape"}, tree) == shape::square);
  CHECK(
    property_parser::read<pair_type>(property_path_type{"pair"}, tree) ==
    pair_type{-3, shape::circle}
  );
  CHECK(property_parser::read<float>(property_path_type{"float"}, tree) == 0.5F);

  // A name of no value of the enum, a pair without one of its values
  property_parser::write(property_path_type{"shape"}, tree, std::string{"line"});
  CHECK(property_parser::read<shape>(property_path_type{"shape"}, tree) == std::nullopt);
  tree.get_child("pair").erase(property_parser::path_name_second);
  CHECK(property_parser::read<pair_type>(property_path_type{"pair"}, tree) == std::nullopt);
}

TEST_CASE("property_parser_reads_no_vector_with_a_missing_value", "[setting]")
{
  using vector_type = std::vector<std::int32_t>;
  auto tree = property_tree_type{};
  const auto path = property_path_type{"list"};
  property_parser::write(path, tree, vector_type{1, 2, 3});
  CHECK(property_parser::read<vector_type>(path, tree) == vector_type{1, 2, 3});
  tree.get_child("list").erase("index_1");
  CHECK(property_parser::read<vector_type>(path, tree) == std::nullopt);
}

} // namespace arcstd::setting
