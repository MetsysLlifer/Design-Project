# DSA Visualizer

An interactive data structures and algorithms visualizer written in C with Raylib and Raygui.

It focuses on teaching pointers, dynamic memory, and different implementation styles such as array-based, linked-list, and cursor-based representations.

## Features

- Interactive visualization of data structures and algorithms
- Logical and physical memory views
- Raygui-powered UI
- Bundled Raylib headers and prebuilt libraries for macOS, Linux x64, and Windows x64

## Project Layout

- `src/` - application source files
- `include/` - project headers
- `vendor/raylib/` - bundled Raylib headers and libraries
- `assets/` - project assets
- `book/` - reference material

## Build

This repo supports CMake and also includes a Makefile.

### CMake

macOS / Linux:

```bash
cmake -S . -B build
cmake --build build --config Release
```

Windows (Visual Studio):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Makefile

```bash
make
```

## Run

The executable is created at:

- `bin/dsa_visualizer` on macOS/Linux
- `bin/Release/dsa_visualizer.exe` or the generated Visual Studio output on Windows, depending on the generator used

## Notes

- `raygui.h` is already bundled in `include/raygui.h`.
- Raylib prebuilt files are included under `vendor/raylib/` so users can clone and build without extra setup.
- The project specification is documented in `PROJECT_SPEC.md`.
