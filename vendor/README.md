
This project can optionally use a vendored prebuilt Raylib to avoid installing system packages and to enable cross-platform builds with CMake.

Place a prebuilt Raylib distribution under vendor/raylib with the following structure if you cannot or do not want to install raylib system-wide:

- vendor/raylib/
  - include/        # contains raylib.h and other headers
  - lib/            # contains libraylib.a / libraylib.dylib / raylib.lib / raylib.dll

Platform notes:

- macOS: put `libraylib.a` or `libraylib.dylib` in `vendor/raylib/lib` and headers in `vendor/raylib/include`.
- Linux: same layout works; put `.a` or `.so` into `vendor/raylib/lib`.
- Windows: put `.lib` import libraries (and optionally `.dll`) into `vendor/raylib/lib` and headers into `vendor/raylib/include`.

CMake support:

- A `CMakeLists.txt` has been added to the project root. It will first try to find an installed raylib package. If not found, it will look for `vendor/raylib` and link any libraries found there.
- You can also pass `-DRAYLIB_DIR=/path/to/raylib` to `cmake` to point to a custom raylib layout.

Makefile support:

- The original `Makefile` prefers `pkg-config --cflags --libs raylib` when available. If not found, it now falls back to `vendor/raylib` as well.

raygui:

- `raygui.h` is bundled in `include/raygui.h` and is enabled by `#define RAYGUI_IMPLEMENTATION` in `src/main.c`.

Building with CMake examples:

macOS / Linux:

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

Windows (MSVC):

```powershell
mkdir build; cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```
 
Prebuilt binaries included in this repository

For convenience, this repository includes prebuilt raylib headers and libraries (static + shared) under `vendor/raylib`:

- `vendor/raylib/include/` — raylib headers (`raylib.h`, `raymath.h`, `rlgl.h`) (~488K)
- `vendor/raylib/lib/` — platform subfolders (~18M total):
  - `linux/` — `libraylib.a` (2.7M)
  - `macos/` — `libraylib.a` (4.8M), `libraylib.6.0.0.dylib` (3.6M)
  - `windows/` — `raylib.lib` (5.3M), `raylib.dll` (1.9M), `raylibdll.lib` (0.4M)

These files are provided so users can clone and build on macOS, Linux x64, and Windows x64 without installing raylib system-wide.

