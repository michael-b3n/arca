#include <arcstd/util/no_destructor.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>

namespace arcstd::util
{
namespace
{

///
/// Counts its destructions in \p destroyed.
///
struct counted final
{
  int& destroyed;
  std::string text;

  counted(int& destroyed_, std::string text_);
  counted(const counted&) = delete;
  counted(counted&&) = delete;
  auto operator=(const counted&) -> counted& = delete;
  auto operator=(counted&&) -> counted& = delete;
  ~counted() noexcept;
};

///
///
counted::counted(int& destroyed_, std::string text_)
  : destroyed{destroyed_}
  , text{std::move(text_)}
{
}

///
///
counted::~counted() noexcept
{
  ++destroyed;
}

} // namespace

TEST_CASE("no_destructor_never_destroys_what_it_holds", "[util]")
{
  auto destroyed = 0;
  {
    auto held = no_destructor<counted>{destroyed, "held"};
    CHECK(held->text == "held");
    (*held).text = "changed";
    CHECK(held->text == "changed");
  }
  CHECK(destroyed == 0);
}

} // namespace arcstd::util
