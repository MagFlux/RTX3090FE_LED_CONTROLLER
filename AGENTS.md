# AGENTS.md

## Project Overview

**RTX3090Controller** is a Windows x64-only desktop GUI application that controls the LED strip on an **NVIDIA GeForce RTX 3090 Founders Edition** GPU. It bypasses GeForce Experience by talking directly to the GPU driver through the NVIDIA NVAPI client illumination zone interface (`nvapi64.dll`, loaded at runtime via `LoadLibraryW` — no NVAPI SDK, no link stubs).

- **Language:** C++17, Qt 5/6 Widgets (QMainWindow, QTabWidget, QSlider, QColorDialog — no QML/QtQuick)
- **Build system:** qmake (not CMake)
- **Dependencies:** Qt Core/Gui/Widgets, MSVC or MinGW 64-bit, `nvapi64.dll` from an installed NVIDIA driver
- **Hardware binding:** PCI vendor `0x10DE`, device `0x2204`, subsystem `0x147D` — hard-binds to RTX 3090 FE

**Source of truth:** `README.md` contains full documentation, build/run instructions, NVAPI interface-ID table, ABI-safety notes, and troubleshooting. Read it before non-trivial changes.

## Build & Run

**Windows only.** The code unconditionally includes `<windows.h>`; Linux/WSL builds will not compile. Do not attempt or "fix" Linux builds.

```bat
qmake RTX3090Controller.pro
nmake
```

Or open `RTX3090Controller.pro` in Qt Creator and build with a desktop kit. Output: `RTX3090Controller.exe`.

The Windows-only `.pro` settings are intentional — do not "generalize" them:
- `INCLUDEPATH += "C:/Windows/System32" "C:/Windows/SysWOW64"`
- `RC_ICONS = resources/icon.ico`
- `QMAKE_CXXFLAGS += -Wno-cast-function-type` (needed for function-pointer casts)

## Project Layout

| File | Purpose |
|---|---|
| `main.cpp` | Entry point: `QApplication`, window icon `:/icon.ico`, shows `MainWindow` |
| `mainwindow.h` / `mainwindow.cpp` | Qt Widgets GUI: tabs, sliders, mode combo, Apply/Cancel slots |
| `nvidiacontroller.h` | **ABI-locked NVAPI struct/typedef definitions (verbatim from NVIDIA headers)** + `NVIDIAController` class declaration + `static_assert` guards |
| `nvidiacontroller.cpp` | DLL loading, `nvapi_QueryInterface` resolution, RTX 3090 FE detection, zone Get/Set, `updateLEDs()` |
| `RTX3090Controller.pro` | qmake project file |
| `resources.qrc` / `resources/icon.ico` | Qt resource + application icon |

### Generated artifacts (do NOT edit or commit)

`Makefile`, `*.o`, `qrc_resources.cpp`, `.qmake.stash`, `build/`, `.qtcreator/` — all gitignored outputs from qmake/Qt Creator.

## Testing & Linting

- **No tests exist.** No QTest, CTest, or CI. Validation is manual on real hardware.
- **No lint/format configs.** No `.clang-format`, `.clang-tidy`, or `.editorconfig`.
- The **only** automated guard for the NVAPI ABI is the set of `static_assert`s in `nvidiacontroller.h`. Do not weaken or remove them.

## Hard Rules (ABI & Safety)

1. **Never reorder, rename, add, or remove fields** in the `NV_GPU_CLIENT_ILLUM_ZONE*` structs in `nvidiacontroller.h`. They are byte-layout-locked to the `nvapi64.dll` ABI; the `static_assert`s (zone size = 200 bytes, params = 6476 bytes, version = 72012u) exist to catch drift. A silent mismatch causes heap overruns at runtime on real hardware — no warning, no exception.
2. **Never change the `nvapi_QueryInterface` typedefs** so the `interfaceID` / return types return `NV_STATUS` or a 32-bit handle. They must remain `void*` function pointers; a 32-bit int truncates on x64 → garbage calls.
3. **Always call `NvAPI_Initialize()` before any other NVAPI call.**
4. **Resolve new NVAPI functions only in `setupQueryInterfaceFunction()`** in `nvidiacontroller.cpp`, using the documented interface IDs. Do not guess interface IDs.
5. **Check `NV_STATUS != NVAPI_OK` after every NVAPI call.**
6. **Do NOT introduce exceptions.** Error handling is boolean returns + `qWarning()`/`qDebug()` logging + `QMessageBox::warning` for user-facing failures.

## Coding Conventions

- 4-space indentation, Qt Creator default brace style; braces on new line for methods.
- `camelCase` for members, locals, and slots (`rgbColorButton`, `onApplyClicked`).
- `UpperCamelCase` for classes (`MainWindow`, `NVIDIAController`).
- `UPPER_SNAKE_CASE` for constants/macros; `NV_`-prefixed names for NVAPI types copied verbatim from NVIDIA headers.
- `Q_OBJECT` macro on all widget/controller classes; use function-pointer `connect()` (not SIGNAL/SLOT macros).
- Use `nullptr`, not `NULL`. Use `QColor`/`QString` for UI data, not raw C types.
- `qDebug() <<` for step-by-step tracing; `qWarning() <<` for failures on apply path.
- `dumpDLLExports()` in `nvidiacontroller.cpp` is a dev-only diagnostic helper.

## Commit Style

Past-tense imperative first line, e.g. "Add brightness slider", "Fix zone detection". Keep messages concise.

## License

None specified. "Use at your own risk" per `README.md`.
