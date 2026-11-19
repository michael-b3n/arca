#include "test_utils/files.hpp"
#include "test_utils/temp_folder.hpp"

#include <arcstd/meta/chrono.hpp>
#include <arcstd/setting/property.hpp>
#include <arcstd/setting/property_node.hpp>
#include <arcstd/setting/property_parser.hpp>
#include <arcstd/setting/property_tree.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <variant>
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

template<typename T>
inline constexpr bool is_optional_v = std::is_same_v<T, std::optional<typename T::value_type>>;
template<typename T>
inline constexpr bool is_vector_v = std::is_same_v<T, std::vector<typename T::value_type>>;

///
/// \return a value of \p T other than its default one, an optional or a vector holds some
///
template<typename T>
[[nodiscard]] auto sample() -> T
{
  if constexpr(std::is_same_v<T, bool>)
  {
    return true;
  }
  else if constexpr(std::is_same_v<T, std::string>)
  {
    return "text";
  }
  else if constexpr(std::is_same_v<T, std::filesystem::path>)
  {
    return "folder/file";
  }
  else if constexpr(meta::is_duration_v<T>)
  {
    return T{5};
  }
  else if constexpr(std::is_arithmetic_v<T>)
  {
    return T{7};
  }
  else if constexpr(is_optional_v<T>)
  {
    return T{sample<typename T::value_type>()};
  }
  else if constexpr(is_vector_v<T>)
  {
    return T{sample<typename T::value_type>(), typename T::value_type{}};
  }
}

///
/// Call \p check with each type of setting_type_erased_variant, as index and as type.
///
template<typename F>
auto for_each_erased_type(const F& check) -> void
{
  [&]<std::size_t... I>(std::index_sequence<I...>)
  {
    (check.template operator()<I, std::variant_alternative_t<I, setting_type_erased_variant>>(),
     ...);
  }(std::make_index_sequence<std::variant_size_v<setting_type_erased_variant>>{});
}

///
/// \return the value of \p node, if it holds one of type \p T
///
template<typename T>
[[nodiscard]] auto read_as(const property_node& node) -> std::optional<T>
{
  return node.read(setting_type_erased_variant{std::in_place_type<T>})
    .transform([](const auto& stored) { return std::get<T>(stored); });
}

} // namespace

TEST_CASE("property_node_keeps_every_type_in_the_settings_file", "[setting]")
{
  const auto folder =
    test_utils::temp_folder{"property_node_keeps_every_type_in_the_settings_file"};
  const auto file = folder.path() / "settings.xml";
  const auto node = [&](const auto& tree, const std::string& name, const std::size_t index)
  { return property_node{tree, std::format("settings.{}.type_{}", name, index)}; };
  {
    const auto tree = property_tree::create(file);
    for_each_erased_type(
      [&]<std::size_t I, typename T>
      {
        node(tree, "sample", I)
          .write(setting_type_erased_variant{std::in_place_type<T>, sample<T>()});
        node(tree, "default", I).write(setting_type_erased_variant{std::in_place_type<T>, T{}});
      }
    );
  }
  // Read from the file again
  const auto tree = property_tree::create(file);
  for_each_erased_type(
    [&]<std::size_t I, typename T>
    {
      INFO("type " << I);
      // Compared as optionals, an optional T would else compare its value to no value
      CHECK(read_as<T>(node(tree, "sample", I)) == std::optional<T>{sample<T>()});
      CHECK(read_as<T>(node(tree, "default", I)) == std::optional<T>{T{}});
      CHECK(read_as<T>(node(tree, "missing", I)) == std::nullopt);
    }
  );
}

TEST_CASE("property_node_reads_no_value_of_another_type", "[setting]")
{
  const auto folder = test_utils::temp_folder{"property_node_reads_no_value_of_another_type"};
  const auto tree = property_tree::create(folder.path() / "settings.xml");
  auto node = property_node{tree, "settings.value"};
  node.write(setting_type_erased_variant{std::string{"text"}});
  CHECK(read_as<std::int32_t>(node) == std::nullopt);
  CHECK(read_as<std::optional<bool>>(node) == std::nullopt);
  CHECK(read_as<std::vector<std::string>>(node) == std::nullopt);
  CHECK(read_as<std::string>(node) == "text");

  CHECK_THROWS_AS((property_node{tree, ""}), util::exception);
}

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
  tree.get_child("pair").erase("second");
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
  {
    const auto tree = property_tree::create(file);
    const auto name = property<std::string>{
      property_node{tree, "settings.name"},
      std::string{"default"}
    };
    CHECK(name.value() == "default");
  }
  CHECK(test_utils::read_file(file).contains("<name>default</name>"));
}

} // namespace arcstd::setting
