# Building StorLive

## Windows x64

Use an MSVC 2022 developer shell with CMake/Ninja and Qt 6.8 available in `PATH`/`CMAKE_PREFIX_PATH`.

```powershell
./packaging/windows/prepare-obs-runtime.ps1 -Version 30.0.2 -WorkDir .obs-runtime
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON -DSTORLIVE_OBS_ROOT="$pwd/.obs-runtime/sdk" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=cl
cmake --build build
cmake --install build --prefix dist
./packaging/windows/make-portable.ps1 -InstallDir dist -OutputDir artifacts -ObsRuntimeRoot .obs-runtime/runtime
```

The production CMake configuration fails if libobs cannot be found. Use `STORLIVE_WITH_LIBOBS=OFF` only for deliberate developer UI/scaffold experiments, never for release packaging.

## Ubuntu 24.04 / Debian-compatible

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev libobs-dev obs-plugins
cmake -S . -B build -G Ninja -DSTORLIVE_WITH_LIBOBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && cpack -G DEB
```
