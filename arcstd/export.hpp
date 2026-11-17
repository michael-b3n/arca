#pragma once

///
/// ARCSTD_<LIBRARY>_EXPORT marks what a shared library of arcstd exports, the rest stays hidden in
/// it. Each library exports its own and imports the others'. A static build defines
/// ARCSTD_STATIC_DEFINE for the libraries and their users, so one header serves both kinds.
///
#ifdef ARCSTD_STATIC_DEFINE
  #define ARCSTD_EXPORT_SYMBOL
  #define ARCSTD_IMPORT_SYMBOL
#elifdef _WIN32
  #define ARCSTD_EXPORT_SYMBOL __declspec(dllexport)
  #define ARCSTD_IMPORT_SYMBOL __declspec(dllimport)
#else
  #define ARCSTD_EXPORT_SYMBOL __attribute__((visibility("default")))
  #define ARCSTD_IMPORT_SYMBOL __attribute__((visibility("default")))
#endif

#ifdef arcstd_core_EXPORTS
  #define ARCSTD_CORE_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_CORE_EXPORT ARCSTD_IMPORT_SYMBOL
#endif

#ifdef arcstd_setting_EXPORTS
  #define ARCSTD_SETTING_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_SETTING_EXPORT ARCSTD_IMPORT_SYMBOL
#endif

#ifdef arcstd_lua_EXPORTS
  #define ARCSTD_LUA_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_LUA_EXPORT ARCSTD_IMPORT_SYMBOL
#endif

#ifdef arcstd_cache_EXPORTS
  #define ARCSTD_CACHE_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_CACHE_EXPORT ARCSTD_IMPORT_SYMBOL
#endif

#ifdef arcstd_web_EXPORTS
  #define ARCSTD_WEB_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_WEB_EXPORT ARCSTD_IMPORT_SYMBOL
#endif

#ifdef arcstd_io_EXPORTS
  #define ARCSTD_IO_EXPORT ARCSTD_EXPORT_SYMBOL
#else
  #define ARCSTD_IO_EXPORT ARCSTD_IMPORT_SYMBOL
#endif
