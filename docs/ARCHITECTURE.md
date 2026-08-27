# Architecture

StorLive is a Qt Quick/C++ desktop application that embeds libobs as the media engine while keeping its own small multi-live interface.

## Runtime layers

1. **Qt/QML UI** — destination configuration, source list, source properties, preview, profile and live controls.
2. **AppController** — application state exposed to QML.
3. **SceneManager** — creates the `Gameplay` libobs scene, capture/audio sources, default composition, source properties and source controls.
4. **ObsPreviewProvider** — receives a scaled BGRA view from libobs' video output for the UI preview. It does not create a second encoder.
5. **MultiOutputManager** — validates destinations, groups destinations by encode profile, creates H.264/AAC encoders and attaches independent RTMP outputs/services.
6. **ObsEngine** — starts libobs, configures 1080p60/48 kHz base media, resolves portable/system data paths and loads modules.

## Multi-output model

Destinations with the same `EncodeProfile::signature()` share one video encoder and one audio encoder. Each destination still owns an independent `rtmp_output` and `rtmp_custom` service, so transport/reconnection failures remain isolated per destination.

The output profile can be 1080p or 720p at 30/60 fps. The base compositor remains 1080p60; per-profile scaling and 30 fps frame division happen at the encoder.

## Encoder selection

- **Automático**: hardware H.264 when a compatible registered encoder is found, otherwise x264/fallback H.264.
- **Hardware**: requires NVENC, AMF, QSV or another recognized hardware H.264 backend.
- **Software (x264)**: prefers `obs_x264`.

AAC is selected from the registered libobs encoders, preferring `ffmpeg_aac`.

## Windows portable

Windows CI uses OBS Studio/libobs 30.0.2 for ABI consistency with Ubuntu 24.04's libobs 30.x baseline. `prepare-obs-runtime.ps1` downloads the official OBS portable runtime and source headers, generates an MSVC import library from the official `obs.dll` exports and prepares a small SDK layout consumed by StorLive CMake.

`make-portable.ps1` keeps StorLive's own Qt deployment and embeds libobs plus only the capture/RTMP/encoder plugins used by the application. It validates that `storlive.exe` imports `obs.dll`, preventing a UI-only stub from being published as a production portable build.

At runtime `ObsEngine` resolves `data/libobs` and `obs-plugins/64bit` relative to `storlive.exe`, so double-click launching does not depend on the current working directory or on OBS Studio being installed.

## Linux

Ubuntu/Debian builds use the system `libobs-dev` and `obs-plugins`. The `.deb` therefore depends on OBS plugins rather than embedding another copy of libobs.

## Source defaults

Visual capture sources are given useful initial scene layouts: display/window/game sources fit the 1920×1080 canvas, and webcam starts as a 480×270 lower-right overlay. Audio-only sources are added without visual transforms.

## Security

Legacy stream keys are not migrated. Credentials entered in the current application live in process memory only and are not written to repository files.
