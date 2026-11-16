#pragma once

#include <arcstd/execution/scope.hpp>
#include <arcstd/signal/queued_connections.hpp>
#include <arcstd/util/non_owning_ptr.hpp>

#include <QList>
#include <QObject>
#include <qtmetamacros.h>
#include <QtQml/qqmlregistration.h>
#include <QUrl>
#include <QVariantList>

#include <filesystem>
#include <memory>
#include <vector>

namespace arcstd::lua
{
class lua_engine;
} // namespace arcstd::lua

namespace arcqml
{

///
/// QML bridge for lua_engine.
/// This lists the loaded user scripts, imports script files and loads the scripts anew.
///
class BridgeScript final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool loading MEMBER loading_ NOTIFY loadingChanged FINAL)
  Q_PROPERTY(QVariantList scripts MEMBER scripts_ NOTIFY scriptsChanged FINAL)

  // Variables
  const std::shared_ptr<arcstd::lua::lua_engine> luaEngine_;
  bool loading_{false};
  QVariantList scripts_;
  arcstd::signal::queued_connections connections_;
  arcstd::execution::scope scope_;

public: // Structors
  explicit BridgeScript(
    std::shared_ptr<arcstd::lua::lua_engine> luaEngine,
    arcstd::util::non_owning_ptr<QObject> parent = nullptr
  );
  ~BridgeScript() noexcept override;

public: // Modifiers
  ///
  /// Load the user scripts anew, \see lua::lua_engine::load_scripts. A call made while they load is
  /// ignored.
  ///
  Q_INVOKABLE void loadScripts();

  ///
  /// Copy the script \p files into the script folder and load the scripts anew, \see
  /// lua::lua_engine::import_scripts. The files are expected as the URLs a file dialog reports. A
  /// call made while the scripts load is ignored.
  ///
  Q_INVOKABLE void importFiles(const QList<QUrl>& files);

  ///
  /// Disconnect all signal connections.
  /// This will stop the frontend backend communication.
  ///
  void disconnect();

signals:
  void loadingChanged();
  void scriptsChanged();

private: // Implementation
  void load(std::vector<std::filesystem::path> files);
  void updateScripts();
};

} // namespace arcqml
