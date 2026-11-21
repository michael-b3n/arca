#pragma once

#include "test_utils/wait_until.hpp"

#include <QCoreApplication>

#include <functional>

namespace arcqml::test_utils
{

///
/// Process the events of the application until \p condition holds. The types under test post
/// their notifications to it, from the threads arcstd notifies on.
/// \return false if it does not within a few seconds
///
[[nodiscard]] inline auto processEventsUntil(const std::function<bool()>& condition) -> bool
{
  return arcstd::test_utils::wait_until(
    [&]
    {
      QCoreApplication::processEvents();
      return condition();
    }
  );
}

} // namespace arcqml::test_utils
