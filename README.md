# Variadic Collision Dynamics
An SKSE plugin to dynamically switch collision presets based on player/NPC action and environment.

## Prerequisites

Download CommonLibVR, check out the `ng` branch, then set its location in the `COMMONLIB_SSE_FOLDER` environment variable.

```powershell
git clone --recursive https://github.com/alandtse/CommonLibVR.git
cd CommonLibVR
git checkout ng
```

## Build

Run CMake from a Visual Studio x64 developer environment, or open the project in Visual Studio/Visual Studio Code with CMake support.

```powershell
cmake --preset debug
cmake --build build/debug
```
