#include "test_utils/temp_folder.hpp"

#include <arcstd/meta/chrono.hpp>
#include <arcstd/meta/pack.hpp>
#include <arcstd/setting/property_node.hpp>
#include <arcstd/setting/property_tree.hpp>
#include <arcstd/setting/setting_common.hpp>
#include <arcstd/setting/setting_validator.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::setting
{
namespace
{

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
  else if constexpr(detail::is_optional_v<T>)
  {
    return T{sample<typename T::value_type>()};
  }
  else if constexpr(detail::is_vector_v<T>)
  {
    return T{sample<typename T::value_type>(), typename T::value_type{}};
  }
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

TEMPLATE_LIST_TEST_CASE(
  "property_node_keeps_every_type_in_the_settings_file", "[setting]", setting_type_erased_variant
)
{
  const auto folder = test_utils::temp_folder{std::format(
    "property_node_keeps_type_{}", meta::type_index_v<setting_type_erased_variant, TestType>
  )};
  const auto file = folder.path() / "settings.xml";
  {
    const auto tree = property_tree::create(file);
    property_node{tree, "settings.sample"}.write(
      setting_type_erased_variant{std::in_place_type<TestType>, sample<TestType>()}
    );
    property_node{tree, "settings.default"}.write(
      setting_type_erased_variant{std::in_place_type<TestType>, TestType{}}
    );
  }
  // Read from the file again. Compared as optionals, an optional T would else compare its value to
  // no value.
  const auto tree = property_tree::create(file);
  CHECK(
    read_as<TestType>(property_node{tree, "settings.sample"}) ==
    std::optional<TestType>{sample<TestType>()}
  );
  CHECK(
    read_as<TestType>(property_node{tree, "settings.default"}) ==
    std::optional<TestType>{TestType{}}
  );
  CHECK(read_as<TestType>(property_node{tree, "settings.missing"}) == std::nullopt);
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

} // namespace arcstd::setting
