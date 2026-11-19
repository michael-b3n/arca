#include <catch2/catch_session.hpp>

#include <QCoreApplication>

///
/// Run the tests under a QCoreApplication, the types under test post their notifications to its
/// event loop, \see arcqml::test_utils::processEventsUntil.
/// \return the result of the test session
///
int main(int argc, char* argv[])
{
  const auto application = QCoreApplication{argc, argv};
  return Catch::Session{}.run(argc, argv);
}
