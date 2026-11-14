# arca

[![CI](https://github.com/michael-b3n/arca/actions/workflows/ci.yml/badge.svg)](https://github.com/michael-b3n/arca/actions/workflows/ci.yml)

General C++26 objects and Qml bindings for applications.

## Repository

| Folder | Content |
|---|---|
| `arcstd` | The C++ library: the workflow base and the settings, script, cache and web workflows (`workflow`), cache and web fetch (`core`), Lua scripting (`lua`), threading and settings (`framework`, `execution`), signals (`signal`), csv and zip readers (`io`), math, type traits and general utilities (`math`, `meta`, `util`, `txt`), the file system (`system`, Windows in `system/windows`) |
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

A project includes the public headers by `#include <arcstd/...>`. They come in two kinds:

- The interface: the workflow layer `arcstd/workflow/` and the general helpers in `meta`, `math`, `util`, `txt`, `io`
  and `system`. A project names the types it works with by the typedefs of the workflows, e.g.
  `workflow_ground::scope_type`, `workflow_settings::validator_type<T>` or `workflow_script::table_type<...>`.
- What the templates of the workflows need: `framework`, `signal`, `execution` and `lua`. They are reached through
  the typedefs, not included directly.

The private headers lie beside them: the cache and the web fetch (`core`), the Lua state, the property tree of the
settings file and the execution library. They are listed in `arcstd/CMakeLists.txt` and refuse to be included by a
project, with an `#error`.

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
