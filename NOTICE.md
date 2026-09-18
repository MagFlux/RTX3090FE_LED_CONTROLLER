# NOTICE — Dependency Licenses and Compliance

This file lists all dependencies of **RTX3090Controller** and explains why
each is compliant with the project license **AGPL-3.0-or-later**
(see `LICENSE.md`).

AI agents and contributors: update this file whenever you add, upgrade,
remove, or vendor any dependency, snippet, header, asset, or generated code.

## Project license

- **RTX3090Controller own code** (`main.cpp`, `mainwindow.h/.cpp`,
  `nvidiacontroller.cpp`, build files, docs) — `AGPL-3.0-or-later`.
  Each source file should carry the AGPL header and
  `SPDX-License-Identifier: AGPL-3.0-or-later`.

## Dependencies

| # | Dependency | Version / source | License | How used | Distributed? | Why compliant |
|---|------------|------------------|---------|----------|--------------|---------------|
| 1 | Qt Core, Gui, Widgets | Qt 5 or Qt 6, system-installed MSVC/MinGW kit (`QT += core widgets gui` in `RTX3090Controller.pro`) | LGPLv3 (also available GPLv3 / commercial; this project uses the LGPL option) | Dynamically linked GUI framework (`QApplication`, `QMainWindow`, `QTabWidget`, `QSlider`, `QColorDialog`, etc.) | No — only source references; user builds against their own Qt install. Binaries link dynamically to Qt DLLs at runtime. | LGPLv3 permits dynamic linking from AGPL code without relicensing Qt. No Qt sources are copied, modified, or vendored. Binary distributions must keep Qt DLLs separate (dynamic, not static link) and retain this notice. If Qt DLLs are shipped alongside the `.exe`, include a written offer / link to Qt sources per LGPL §6 and allow relinking. |
| 2 | NVIDIA NVAPI header-derived types | Derived from https://github.com/NVIDIA/nvapi (2024), copied verbatim into `nvidiacontroller.h` | MIT (Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES) | Struct/enum/typedef definitions for `NV_GPU_CLIENT_ILLUM_ZONE*` ABI; no NVAPI SDK headers, import libs, or link stubs used | Yes — small MIT-licensed excerpts are included in source | MIT is permissive and compatible with AGPL-3.0-or-later. Compliance is via the preserved MIT copyright + permission notice at the top of `nvidiacontroller.h` plus `SPDX-License-Identifier: MIT` on those portions. The combined work is conveyed under AGPL-3.0-or-later, which MIT permits. Never remove that notice. |
| 3 | `nvapi64.dll` (NVIDIA driver runtime) | Installed by the user's NVIDIA GPU driver in `C:\Windows\System32`; loaded at runtime via `LoadLibraryW` + `nvapi_QueryInterface` | Proprietary (NVIDIA driver EULA) | Runtime-only: 5 functions resolved by interface ID (`NvAPI_Initialize`, `NvAPI_EnumPhysicalGPUs`, `NvAPI_GPU_GetPCIIdentifiers`, `...IllumZonesGetControl/SetControl`). Never linked at build time. | No — never vendored, copied, or shipped with this repo | No distribution = no license conflict. Runtime dynamic loading of a user-installed system driver component is a System Library / mere aggregation boundary, not a derivative-work conveyance. Keep it this way: do not vendor the DLL, SDK headers, or import libs into the repo. |
| 4 | Windows SDK (`windows.h`) + MSVC/MinGW runtimes, C++ standard library | Toolchain-provided (Visual Studio / MinGW-w64) | Proprietary (Windows SDK) / Microsoft STL exception / GPLv3+GCC-exception (libstdc++) as applicable | OS API (`LoadLibraryW`, PE-header parsing) and standard C++ (`std::vector`, `stdint.h`) | No — provided by the build toolchain | Treated as System Libraries / standard toolchain components under AGPL §1. Normal compilation against them does not impose extra source obligations. Do not copy SDK headers into the repo. |
| 5 | `resources/icon.ico` (app icon, via `resources.qrc`, `RC_ICONS`) | Project-local asset in `resources/` | Project-owned (conveyed under AGPL-3.0-or-later like other own code) | Embedded via Qt resources, shown as window icon | Yes — shipped in repo and binary | No third-party license involved. If replaced with a third-party icon, record its license here and verify AGPL compatibility first. |

## What is intentionally NOT a dependency

- No NVAPI SDK download, headers, or `.lib` stubs are required or included.
- No vendored third-party code, package-manager packages, fonts, images, or
  AI-generated snippets beyond the project-owned sources above.
- qmake-generated outputs (`Makefile`, `*.o`, `qrc_resources.cpp`,
  `.qmake.stash`, `build/`, `.qtcreator/`) are build artifacts, not
  dependencies, and must not be committed.

## Rules for future changes (summary)

1. Only add dependencies compatible with `AGPL-3.0-or-later` (e.g. MIT, BSD,
   LGPLv3-dynamic, GPLv3, AGPLv3, public domain). Reject GPLv2-only,
   proprietary static-link, or unknown-license code.
2. Preserve all `SPDX-License-Identifier` tags, copyright headers, and the
   MIT notice in `nvidiacontroller.h`.
3. New source files must carry the AGPL header.
4. Prefer dynamic linking for LGPL components; never static-link Qt without
   adding a source offer here.
5. Never vendor `nvapi64.dll` or NVAPI SDK binaries.
6. Update this table with every dependency change.
