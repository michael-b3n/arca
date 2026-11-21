# Development setup (Windows)

## Tools

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [MSYS2](https://www.msys2.org/).
2. In the `MSYS2 MSYS` shell (`msys2_shell.cmd`), update and install the toolchain, with
   clang-tidy, clangd and gcovr:

   ```
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-{toolchain,cmake,ninja,clang,lld,clang-tools-extra,gcovr}
   ```

3. Install the libraries the project builds against:

   ```
   pacman -S --needed mingw-w64-ucrt-x86_64-{boost,catch,curl,libzip,lua54,qt6-declarative,qt6-svg,sol2,spdlog,sqlite3}
   ```

4. In Visual Studio Code install at least the [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack), [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) and Code Spell Checker.

## Build

`CMakePresets.json` holds a preset per compiler and build type: `clang-debug`, `clang-release`, `gcc-debug` and `gcc-release`, and `gcc-release-static`, which links the tests fully static, and `gcc-coverage`, which measures the coverage of the tests. All build into `build` and install into `install/<preset>`. CI uses `gcc-release`, `gcc-release-static` and `gcc-coverage`.

The presets expect the compilers and Ninja on the PATH, as in the `MSYS2 UCRT64` shell. In the root directory:

```
cmake --preset clang-debug --fresh
cmake --build --preset clang-debug
ctest --preset clang-debug
```

`--fresh` replaces the configuration of the previous preset.

The coverage of the tests: build and test `gcc-coverage` the same way, then run `gcovr` in the root directory. It reads `gcovr.cfg` and prints the lines of arcstd the tests run.

In Visual Studio Code, CMake Tools can build with kits and variants instead, which put the compiler folder on the PATH. Set `"cmake.useCMakePresets": "never"` in `.vscode/settings.json` to keep them while the presets file is present.
