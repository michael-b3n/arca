#pragma once

///
/// ARCSTD_EXPORT marks what a shared arcstd exports, the rest stays hidden in it. A static arcstd
/// defines ARCSTD_STATIC_DEFINE for itself and its users, so one header serves both kinds.
///
#ifdef ARCSTD_STATIC_DEFINE
  #define ARCSTD_EXPORT
#elifdef _WIN32
  #ifdef arcstd_EXPORTS
    #define ARCSTD_EXPORT __declspec(dllexport)
  #else
    #define ARCSTD_EXPORT __declspec(dllimport)
  #endif
#else
  #define ARCSTD_EXPORT __attribute__((visibility("default")))
#endif
