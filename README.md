# arca

[![CI](https://github.com/michael-b3n/arca/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/arca/actions/workflows/ci.yml)
[![Coverage](https://codecov.io/gh/michael-b3n/arca/graph/badge.svg)](https://codecov.io/gh/michael-b3n/arca)

arca is a foundation for scriptable desktop applications in C++26. It holds what such an
application needs besides its own domain: settings that are validated and kept in a settings file,
Lua scripts that extend the application and the services they use (a cache that outlives the run,
web requests), logging, work on a thread pool and signals between objects. The QML module `ArcQml`
binds these to a Qt Quick interface: settings, scripts and logs, plus controls, styles and shapes.

The general helpers below them (type traits, math, string and enum utilities) serve applications
and games alike. Code that belongs to one application stays there: arca knows no domain.

## Targets

Each area of `arcstd` is a folder, a namespace and a static library. A library links the ones it
builds on, so a project takes only what it uses:

```
core ─┬─ io
      └─ setting ── lua ─┬─ cache
                         └─ web
```

| Library | Areas | Main components | Third party |
|---|---|---|---|
| `arca::core` | `util`, `meta`, `math`, `signal`, `execution`, `system` | `log`, `exception`, `enum`; `signal::adapter`; `execution::launch` and `scope` | spdlog, magic_enum, Boost, beman.execution |
| `arca::io` | `io` | `csv_reader`, `zip_file_reader` | libzip, rapidcsv |
| `arca::setting` | `setting` | `setting_store` owns the settings and keeps them in the settings file, `setting` and its validators | Boost.PropertyTree |
| `arca::lua` | `lua` | `lua_engine` runs the bundled and the user scripts, `value` and `value_cast` convert between C++ and Lua | Lua, sol2 |
| `arca::cache` | `cache` | `cache_store`, text values of the scripts beyond the run, `cache.get` in Lua | SQLite |
| `arca::web` | `web` | `web_fetcher`, web pages for the scripts, `web.fetch` in Lua | libcurl, Boost.URL |
| `arca::arcstd` | all | all libraries above | |

Headers are included as `<arcstd/<area>/...>`. A helper names the types it hands out as aliases,
e.g. `setting_store::validator_type<T>`. An application calls `execution::shutdown_thread_pool()`
last, once its objects that launch work are gone.

Only the Boost headers and magic_enum reach the public headers, the other third party libraries
stay private. Private headers lie beside the public ones, are listed in `arcstd/CMakeLists.txt`
and refuse to be included by a project.

The other targets, not installed, only with `add_subdirectory`:

| Target | Builds | Links |
|---|---|---|
| `ArcQml` | static library of the QML module `ArcQml` from `arcqml`: its C++ types, controls, styles and shapes, for code that uses them | `arca::lua`, Qt Quick |
| `ArcQmlplugin` | static plugin of the module, an application links it so QML finds `import ArcQml` | `ArcQml` |
| `arcstd_test_utils` | test helpers of `arcstd_test/test_utils`, also for the tests of projects built on arca, e.g. `"test_utils/temp_folder.hpp"` | Catch2, `arca::arcstd` |
| `arcstd_test` | tests of the public headers, built like a project on arca | `arcstd_test_utils` |
| `arcstd_private_test` | tests of the internals in `arcstd_test/private`, with the private headers | `arcstd_test_utils`, `arcstd_internals` |
| `arcqml_test` | tests of the C++ types of ArcQml that need no window, under a `QCoreApplication`, with helpers of their own in `arcqml_test/qml_test_utils`; not in the static and coverage builds | `arcstd_test_utils`, `ArcQml` |

The tests and `arcstd_test_utils` are built with `ARCA_BUILD_TESTS`, on by default when arca is the
top level project, as is `ARCA_INSTALL`, which installs the libraries as the package `arca`.

## Use in a project

As subdirectory, e.g. a git submodule:

```cmake
add_subdirectory(arca)
target_link_libraries(app PRIVATE arca::lua ArcQmlplugin)
```

Or installed:

```cmake
find_package(arca REQUIRED)
target_link_libraries(app PRIVATE arca::lua)
```

The libraries are static and pass the third party libraries they use on to the link of the
application. By default those are the shared libraries of MSYS2, as is the C++ runtime of MinGW:
an application ships their DLLs beside itself. For a single executable, build arca with
`ARCA_STATIC_DEPENDENCIES=ON` and link the application with `-static`: the third party libraries
are then their static ones, spdlog header only, and the executable loads DLLs of Windows only. An
installed arca keeps that choice. Qt is the application's: a static ArcQml needs a static Qt.

A static C++ runtime alone (`-static-libgcc -static-libstdc++`) pays only if the application loads
no other C++ DLL. Qt, spdlog and the like bring the runtime as DLL anyway, and next to them an own
copy of the runtime and its unwinder lets exceptions between the two end the program.

`configure_embedded_scripts` of `tools/cmake` compiles Lua scripts into a library with `#embed`.

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
```

`gcc-release-static` links the tests fully static, they also check that they load DLLs of Windows
only; it leaves `arcqml_test` out, the Qt of MSYS2 has DLLs only. All presets build into `build`,
`--fresh` replaces the configuration of the previous one. The target
`all_verify_interface_header_sets` compiles each public header on its own, the target `install`
installs into `install/<preset>`.

`gcc-coverage` builds arcstd and its tests with the coverage instrumentation of GCC, ArcQml is left
out. After its tests, `gcovr` in the repository root reports the lines of arcstd they run, as set
in `gcovr.cfg`. CI sends the report to Codecov, which shows it in the badge above.

A test file tests one object, at the path of its source: `arcstd_test/io/csv_reader.cpp` tests
`arcstd/io/csv_reader.hpp`, the private ones in `arcstd_test/private`. A test is named
`<object>_<what it does>` in snake_case, e.g. `csv_reader_reads_a_file`, a type of ArcQml keeps its
name, e.g. `SettingBinding_binds_the_value_to_the_setting`. Its tag is its area, e.g. `[io]`.
Cases share a setup in a `SECTION`, a test for several types is a `TEMPLATE_LIST_TEST_CASE`, and a
check that throws names the exception with `CHECK_THROWS_AS`. ctest lists the tests before it runs
them.

## License

[MIT](LICENSE)
