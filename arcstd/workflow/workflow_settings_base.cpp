#include "arcstd/workflow/workflow_settings_base.hpp"

namespace arcstd::workflow
{

///
///
workflow_settings_base::workflow_settings_base(std::shared_ptr<setting::setting_store> setting_store)
  : setting_store_{std::move(setting_store)}
{
}

///
///
workflow_settings_base::~workflow_settings_base() noexcept = default;

} // namespace arcstd::workflow
