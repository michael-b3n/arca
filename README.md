# arca

[![CI](https://github.com/michael-b3n/arca/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/arca/actions/workflows/ci.yml)

General C++26 objects and Qml bindings for applications.

## Repository

| Folder | Content |
|---|---|
| `arcstd` | The C++ library: the workflow base and the settings, script, cache and web workflows (`workflow`), cache and web fetch (`core`), Lua scripting (`lua`), `std::execution` helpers (`execution`), threading and settings (`framework`), csv and zip readers (`io`), math, type traits and general utilities (`math`, `meta`, `util`, `txt`), signals (`signal`), the file system (`system`, Windows in `system/windows`) |
| `arcqml` | The Qt layer, QML module `ArcQml`: controls, styles, icons and shapes, and the bindings of the settings, scripts and logs: their bridges and models |
| `arcstd_test` | Catch2 tests of `arcstd`, and `arcstd_test_utils`, the test helpers a project built on arca may use too |
| `libs_external` | Third party sources, used as they are |
| `tools` | CMake helpers and the clang-tidy runner |

## Use in a project

Add arca as a subdirectory, e.g. a git submodule, and link the targets:

```cmake
add_subdirectory(arca)
target_link_libraries(app PRIVATE arcstd ArcQmlplugin)
```

The tests of arca build when it is the top level project, or with `ARCA_BUILD_TESTS=ON`.
`arcstd` is a static library, with `ARCA_BUILD_SHARED=ON` a shared one; the project then places `arcstd.dll` beside
its executables. `ArcQml` is always static.

The interface of `arcstd` is the workflow layer: a project includes `arcstd/workflow/` and names the types it works
with by the typedefs there, e.g. `workflow_ground::scope_type` or `workflow_settings::validator_type<T>`. The
general helpers in `meta`, `math`, `util`, `txt`, `io` and `system/filesystem.hpp` may be included directly.

## Development

Setup: [doc/setup_ide.md](doc/setup_ide.md). In the MSYS2 UCRT64 shell:

```
cmake --preset gcc-release --fresh
cmake --build --preset gcc-release
ctest --preset gcc-release
```

## License

[MIT](LICENSE)
