# arca

[![CI](https://github.com/michael-b3n/arca/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/arca/actions/workflows/ci.yml)

arca is a foundation for scriptable desktop applications in C++26. It holds what such an
application needs besides its own domain: settings that are validated and kept in a settings file,
Lua scripts that extend the application and the services they use (a cache that outlives the run,
web requests), logging, work on a thread pool and signals between objects. The QML module `ArcQml`
binds these to a Qt Quick interface: settings, scripts and logs, plus controls, styles and shapes.

The general helpers below them (type traits, math, string and enum utilities) serve applications
and games alike. Code that belongs to one application stays there: arca knows no domain.

## Libraries

Each area of `arcstd` is a folder, a namespace and a library. A library links the ones it builds
on, so a project takes only what it uses:

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
| `ArcQmlplugin` | `arcqml` | QML module `ArcQml` | Qt |

Headers are included as `<arcstd/<area>/...>`. A helper names the types it hands out as aliases,
e.g. `setting_store::validator_type<T>`. An application calls `execution::shutdown_thread_pool()`
last, once its objects that launch work are gone.

Only the Boost headers and magic_enum reach the public headers, the other third party libraries
stay private. Private headers lie beside the public ones, are listed in `arcstd/CMakeLists.txt`
and refuse to be included by a project. `arcstd_test_utils` holds test helpers, also for the tests
of projects built on arca.

## Use in a project

As subdirectory, e.g. a git submodule:

```cmake
add_subdirectory(arca)
target_link_libraries(app PRIVATE arca::lua ArcQmlplugin)
```

Or installed:

```cmake
find_package(arca REQUIRED)  # or COMPONENTS static / shared
target_link_libraries(app PRIVATE arca::lua)
```

The libraries are static, with `ARCA_BUILD_SHARED=ON` shared; an application then places the
`libarcstd_<name>.dll` it uses beside itself. A static and a shared build install side by side.
`configure_embedded_scripts` of `tools/cmake` compiles Lua scripts into a library with `#embed`.

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
```

`gcc-release-shared` builds shared libraries, into `build-shared`. The target
`all_verify_interface_header_sets` compiles each public header on its own, the target `install`
installs into `install/<config>`.

## License

[MIT](LICENSE)
