# Installation

Bitcoin Core is built with CMake and a C++ compiler. A typical build requires:

- C++ compiler such as GCC or Clang
- [CMake](https://cmake.org/)
- Build tool like [Ninja](https://ninja-build.org/) or [GNU Make](https://www.gnu.org/software/make/)

## Build options

| Build type | Description | CMake option |
|------------|-------------|--------------|
| GUI | Build the Qt-based `bitcoin-qt` graphical interface. | `-DBUILD_GUI=ON` |
| Headless | Build command-line tools and daemon only. | `-DBUILD_GUI=OFF` |

For detailed platform-specific instructions, see [doc/build-\*.md](doc).
