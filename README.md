# arca

[![CI](https://github.com/michael-b3n/arca/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/arca/actions/workflows/ci.yml)

General C++26 objects and Qml bindings for applications.

## Repository

| Folder | Content |
|---|---|
| `arcstd` | The C++ library, a collection of helpers by area, see [Use in a project](#use-in-a-project) |
| `arcqml` | The Qt layer, QML module `ArcQml`: controls, styles, icons and shapes, and the bindings of the settings, scripts and logs: their bridges and models |
| `arcstd_test` | Catch2 tests of `arcstd`: `arcstd_test` uses it like a project does, `arcstd_private_test` (folder `private`) tests the internals. And `arcstd_test_utils`, the test helpers a project built on arca may use too |
| `libs_external` | Third party sources, used as they are |
| `tools` | CMake helpers and the clang-tidy runner |

## Use in a project

Add arca as a subdirectory, e.g. a git submodule, and link the targets:

```cmake
add_subdirectory(arca)
target_link_libraries(app PRIVATE arca::arcstd ArcQmlplugin)
```

The tests of arca build when it is the top level project, or with `ARCA_BUILD_TESTS=ON`.
`arcstd` is a static library, with `ARCA_BUILD_SHARED=ON` a shared one; the project then places `libarcstd.dll` beside
its executables. `ArcQml` is always static.

A project includes the headers of an area by `#include <arcstd/<area>/...>`, the folder is the namespace:

| Area | Helpers |
|---|---|
| `setting` | `setting_store` owns the settings and keeps them in the settings file; the settings, their validators and type erased views |
| `lua` | `lua_engine` owns the Lua state, loads the scripts and runs their functions; the values, tables and functions of scripts |
| `cache` | `cache_store` keeps text values beyond the run, for scripts as `cache.get` and `cache.set` |
| `web` | `web_fetcher` fetches web pages, for scripts as `web.fetch` |
| `signal` | Signals, their adapter and connections, `queued_connections` run slots on the thread pool |
| `execution` | `scope` and `launch`, work on the thread pool |
| `io` | csv and zip readers |
| `system` | The file system |
| `meta`, `math`, `util` | Type traits, math and general utilities |

A helper object names the types it hands out as type aliases, e.g. `setting_store::validator_type<T>` or
`lua_engine::table_type<...>`.

The private headers lie beside the public ones: the database of a cache, the web request, the Lua state, the property
tree of the settings file and the execution library. They are listed in `arcstd/CMakeLists.txt` and refuse to be
included by a project, with an `#error`.

A shared `arcstd` exports what is marked `ARCSTD_EXPORT`. Third party libraries in the public headers are the Boost
headers, magic_enum and incbin; Lua, sol2 and the others are private.

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
```

The preset `gcc-release-shared` builds `arcstd` as a shared library, into `build-shared`.
The target `all_verify_interface_header_sets` compiles each public header on its own, with the include paths a project
gets: none may include a private header.

## License

[MIT](LICENSE)
