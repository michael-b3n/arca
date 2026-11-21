#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

namespace arcstd::util
{
namespace
{

///
/// \return the report of \p thrown, thrown and caught
///
template<typename T>
[[nodiscard]] auto report_of(const T& thrown) -> std::string
{
  try
  {
    // NOLINTNEXTLINE(bugprone-std-exception-baseclass) the report tells any thrown type
    throw thrown;
  }
  catch(...)
  {
    return exception_report();
  }
}

} // namespace

TEST_CASE("exception_keeps_its_message_and_location", "[util]")
{
  const auto line = std::source_location::current().line() + 1;
  const auto ex = exception{"failed"};
  CHECK(std::string_view{ex.what()} == "failed");
  CHECK(ex.where().line() == line);
  CHECK(std::string_view{ex.where().file_name()}.ends_with("exception.cpp"));

  const auto text = std::format("{}", ex);
  CHECK(text.starts_with("failed\n  file: "));
  CHECK(text.contains(std::format(":{}:", line)));
  CHECK(text.contains("\n  function: "));
  CHECK(text.contains("\n  stack: "));
}

TEST_CASE("exception_report_tells_the_exception_being_handled", "[util]")
{
  CHECK(report_of(exception{"failed"}).starts_with("failed\n  file: "));
  CHECK(report_of(std::runtime_error{"standard"}) == "standard");
  CHECK(report_of(1) == "unknown exception");
  CHECK(exception_report().starts_with("exception_report called with no active exception"));
}

} // namespace arcstd::util
