#pragma once

#include <catch2/catch_tostring.hpp>

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <string>

///
/// Catch prints the Qt values a failed check compares, as qDebug writes them.
///
template<>
struct Catch::StringMaker<QString>
{
  ///
  /// \return \p value as Catch prints it
  ///
  [[nodiscard]] static auto convert(const QString& value) -> std::string;
};

template<>
struct Catch::StringMaker<QStringList>
{
  ///
  /// \return \p value as Catch prints it
  ///
  [[nodiscard]] static auto convert(const QStringList& value) -> std::string;
};

template<>
struct Catch::StringMaker<QVariant>
{
  ///
  /// \return \p value as Catch prints it
  ///
  [[nodiscard]] static auto convert(const QVariant& value) -> std::string;
};

namespace arcqml::test_utils
{

///
/// \return \p value as qDebug writes it
///
template<typename T>
[[nodiscard]] auto debugText(const T& value) -> std::string
{
  auto text = QString{};
  QDebug{&text} << value;
  return text.trimmed().toStdString();
}

} // namespace arcqml::test_utils

///
///
inline auto Catch::StringMaker<QString>::convert(const QString& value) -> std::string
{
  return arcqml::test_utils::debugText(value);
}

///
///
inline auto Catch::StringMaker<QStringList>::convert(const QStringList& value) -> std::string
{
  return arcqml::test_utils::debugText(value);
}

///
///
inline auto Catch::StringMaker<QVariant>::convert(const QVariant& value) -> std::string
{
  return arcqml::test_utils::debugText(value);
}
