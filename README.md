# Standalone RTX 3090 FE LED Controller

This is a simplified standalone Qt application that controls only the NVIDIA RTX 3090 FE LED strip.

## Features
- Simple GUI for controlling RTX 3090 FE LED colors
- Direct communication with NVIDIA GPU using NVAPI
- Real-time color adjustment
- Brightness control

## Build Instructions

### Linux:
1. Install dependencies:
   - `sudo apt install qt5-default libnvapi-dev`

2. Build:
   ```bash
   qmake RTX3090Controller.pro
   make
   ```

### Windows:
1. Install Qt with Visual Studio support
2. Install NVIDIA NVAPI SDK
3. Build using Qt Creator or qmake

### macOS:
1. Install Qt and Xcode
2. Install NVIDIA NVAPI SDK
3. Build using Qt Creator or qmake