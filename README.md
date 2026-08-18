# RTX 3090 FE LED Controller

A standalone Windows desktop application that directly controls the LED strip on the
**NVIDIA GeForce RTX 3090 Founders Edition**, bypassing GeForce Experience entirely by
talking to the GPU driver through the NVIDIA NVAPI illumination zone interface.

Built with C++ and Qt (Core + Widgets), compiled for Windows with a qmake/MSVC
toolchain.

## Features

- **RGB color control** — pick any color via a standard color picker, with a live preview swatch.
- **White brightness control** — 0–100% master brightness slider.
- **Off / Direct modes** — switch the LED strip off, or drive it in direct (manual) mode.
- **Apply / Cancel workflow** — change settings freely in the UI; nothing is pushed to the
  GPU until you press **Apply**. **Cancel** discards pending changes.
- **Direct driver access** — no GeForce Experience or other middleware required; the app
  resolves the NVAPI functions it needs from `nvapi64.dll` at runtime.
- **RGBW aware** — on zones the driver reports as RGBW, the white component of the chosen
  color (i.e. `min(R, G, B)`) is routed to the dedicated white diode for a cleaner white,
  while the residual drives the R/G/B channels. RGB-only zones receive the raw color;
  single-color and fixed-color zones receive the brightness value.

## Requirements

- **Windows 10 / 11 x64**
- **GeForce RTX 3090 Founders Edition** — the app enumerates all physical GPUs and binds
  only to the card with PCI vendor ID `0x10DE`, device ID `0x2204` (RTX 3090), and
  subsystem ID `0x147D` (Founders Edition). Other GPUs or AIB partner cards of the same
  GPU will *not* be targeted.
- **A current NVIDIA GPU driver for Windows** that ships `nvapi64.dll` with the client
  illumination zone support (`NvAPI_GPU_ClientIllumZonesGetControl` /
  `NvAPI_GPU_ClientIllumZonesSetControl`).
- **Qt 5 (or Qt 6) with C++ core & widgets**, and a Visual Studio / MSVC toolchain
  (MSVC 2017 or newer recommended).

No NVAPI SDK headers, import libraries, or link stubs are required — the DLL is loaded
dynamically and all functions are resolved through `nvapi_QueryInterface`, so there is
nothing NVAPI-specific to link against.

## Build Instructions

1. Install **Windows**, **Visual Studio** (with the C++ Desktop workload), and
   **Qt 5 or Qt 6** with the corresponding MSVC kit (e.g. `msvc2019_64` /
   `msvc2022_64` and `mingw` is *not* the target of the `.pro` file — use the
   MSVC-wins mode).
2. Open the project:
   - In **Qt Creator**: *File → Open File or Project* → select `RTX3090Controller.pro`,
     pick the MSVC desktop kit, and click **Build**.
   - Or from a **Developer Command Prompt for VS** with Qt in `PATH`:
     ```bat
     qmake RTX3090Controller.pro
     nmake
     ```
3. The executable `RTX3090Controller.exe` is produced in the build directory
   (typically `release\` or `debug\` for a shadow build).
4. The application expects `nvapi64.dll` to be reachable via the standard system search
   path — any up-to-date NVIDIA driver installs it into `C:\Windows\System32`, so for
   normal use you do **not** need to copy it next to the exe.

## Running

1. Launch `RTX3090Controller.exe`.
   - If the RTX 3090 FE is detected and the illumination interfaces are available, the
     window opens and you can begin adjusting settings.
   - If initialization fails, the app shows a warning dialog. Common causes:
     - no NVIDIA driver installed, or a driver older than client-illumination support;
     - the only GPU present is not an RTX 3090 FE (device/subsystem ID mismatch);
     - another program exclusively owns the illumination state (unusual, but
       GeForce Experience / third-party overclocking tools have been known to interfere).
2. Pick a color on the **RGB Color** tab, set the slider on the **White Brightness**
   tab, and choose **Off** or **Direct** in the **Mode** selector.
3. Press **Apply** to push the settings to the GPU, or **Cancel** to reset pending
   UI state.

## How It Works

### Initialization sequence

```
loadNVAPILibrary()          LoadLibraryW("nvapi64.dll")  (falls back to nvapi.dll,
                            then to a couple of well-known paths)
  └─ GetProcAddress(...)    resolve  nvapi_QueryInterface

setupQueryInterfaceFunction()
  └─ NvAPI_QueryInterface( ID ) → NvAPI_Initialize
                               → NvAPI_EnumPhysicalGPUs
                               → NvAPI_GPU_GetPCIIdentifiers
                               → NvAPI_GPU_ClientIllumZonesGetControl
                               → NvAPI_GPU_ClientIllumZonesSetControl

NvAPI_Initialize()          required before any other call
detectRTX3090FE()           enumerate GPUs, match vendor/device/subsystem ID
getZoneInfo()               GetControl → how many illumination zones this card has
```

The NVAPI surface is a *query-interface* (COM-style) API: `nvapi_QueryInterface(interfaceID)`
returns a pointer to the named function. The five functions used by this application are
resolved with the stable interface IDs below — these are the same IDs the NVIDIA
developer headers (`nvapi.h`) expose:

| Function                                   | Interface ID |
|--------------------------------------------|--------------|
| `NvAPI_Initialize`                         | `0x0150E828` |
| `NvAPI_EnumPhysicalGPUs`                   | `0xE5AC921F` |
| `NvAPI_GPU_GetPCIIdentifiers`              | `0x2DDFB66E` |
| `NvAPI_GPU_ClientIllumZonesGetControl`     | `0x3DBF5764` |
| `NvAPI_GPU_ClientIllumZonesSetControl`     | `0x197D065E` |

### Applying a color

`NVIDIAController::updateLEDs()`:

1. Calls `NvAPI_GPU_ClientIllumZonesGetControl` to fetch the live zone control block —
   this preserves fields the driver expects and tells us the `type` and `ctrlMode` of
   each zone. If that call fails, the code falls back to synthesising a single
   manual-RGB zone so Apply still has a chance of working.
2. For each zone:
   - **RGBW**: computes `w = min(R, G, B)` and writes
     `colorR = R - w`, `colorG = G - w`, `colorB = B - w`, `colorW = w`.
   - **RGB**: writes the raw R/G/B channels.
   - **SINGLE_COLOR / COLOR_FIXED**: writes `brightnessPct` only.
   - If **Mode = Off**, all channels are zeroed and `brightnessPct = 0`.
3. Calls `NvAPI_GPU_ClientIllumZonesSetControl` with the updated block.

Note that per NVAPI convention, `brightnessPct` is the *master* percentage for the
zone. In this application the **White Brightness** slider is what feeds that value, so
treat the slider as the master LED brightness; the color picker controls *what* hue is
emitted.

### ABI safety

The zone-control structures are defined in `nvidiacontroller.h` by copying the NVIDIA
public headers verbatim. Correctness is enforced at **compile time** via
`static_assert`s on the exact sizes and version constants:

```cpp
static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE)              == 200,  ...);
static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS) == 6476, ...);
static_assert(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER   == 72012u, ...);
```

If any of these trip, the struct definitions have drifted from the actual `nvapi64.dll`
ABI and the application will refuse to run. **Do not reorder, rename, or pad fields in
those structs** — the driver reads and writes them through system memory and a mismatch
in size or offset is a heap overrun, not a warning.

The header also pins the function-pointer typedefs. In particular,
`nvapi_QueryInterface` returns a **function pointer**, not a 32-bit handle — modeling
it as `NV_STATUS` (a 32-bit unsigned int) truncates the pointer on x64 Windows and
produces garbage call targets.

## Project Layout

| File / Directory        | Purpose                                                                 |
|-------------------------|-------------------------------------------------------------------------|
| `main.cpp`              | Qt entry point — creates `QApplication` and shows the `MainWindow`.     |
| `mainwindow.h/.cpp`     | GUI: color picker, brightness slider, mode selector, Apply / Cancel.    |
| `nvidiacontroller.h`    | NVAPI type definitions (verbatim from NVIDIA headers, ABI-locked) and the `NVIDIAController` class declaration. |
| `nvidiacontroller.cpp`  | DLL loading, query-interface resolution, GPU detection, zone get/set.   |
| `RTX3090Controller.pro` | qmake project file — links against Qt Core/Widgets/Gui, MSVC-only flags. |
| `build/`                | Build artifacts (git-ignored).                                          |
| `.qtcreator/`           | Qt Creator workspace metadata.                                          |

## Troubleshooting

Symptom: **warning dialog on launch** or **Apply silently does nothing**.
1. Verify `nvidia-smi` reports an **RTX 3090** with the matching board (FE).
2. Install the latest NVIDIA driver from NVIDIA for Windows (GeForce).
3. Confirm `nvapi64.dll` is present in `C:\Windows\System32`.
4. Reboot and retry — other software (e.g. GeForce Experience lighting features) can
   hold the illumination control block; closing it usually fixes things.

## License

None specified. Use at your own risk — this project writes directly to GPU driver
state. If anything goes wrong, the GPU LED strip state is typically reset by
rebooting or by restoring via GeForce Experience.
