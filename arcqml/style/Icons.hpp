#pragma once

#include <arcstd/util/non_owning_ptr.hpp>

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace arcqml
{
namespace detail
{

///
/// Helper function to build the source url of an icon resource, \p name is its file name inside the module resource folder.
/// \return The corresponding resource url
///
[[nodiscard]] auto toIconUrl(std::string_view name) -> QString;

} // namespace detail

///
/// QML Icons singleton, the icon sources of the module with the resource prefix in one place.
///
class Icons final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QString add MEMBER add_ CONSTANT)
  Q_PROPERTY(QString dragHandle MEMBER dragHandle_ CONSTANT)
  Q_PROPERTY(QString folderOpen MEMBER folderOpen_ CONSTANT)
  Q_PROPERTY(QString loading MEMBER loading_ CONSTANT)
  Q_PROPERTY(QString remove MEMBER remove_ CONSTANT)

  // Variables
  QString add_{detail::toIconUrl("add.svg")};
  QString dragHandle_{detail::toIconUrl("drag_handle.svg")};
  QString folderOpen_{detail::toIconUrl("folder_open.svg")};
  QString loading_{detail::toIconUrl("loading.svg")};
  QString remove_{detail::toIconUrl("remove.svg")};

public: // Structors
  explicit Icons(arcstd::util::non_owning_ptr<QObject> parent = nullptr);
  ~Icons() noexcept override;
};

} // namespace arcqml
