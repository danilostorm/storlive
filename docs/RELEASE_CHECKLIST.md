# Release checklist

A release is considered publishable only when all of the following are true:

- Linux configures with `STORLIVE_WITH_LIBOBS=ON` and links `libobs.so`.
- Linux `.deb` is generated successfully.
- Windows prepares OBS/libobs 30.0.2 runtime and SDK from the official OBS release.
- Windows configures with `STORLIVE_WITH_LIBOBS=ON`; production CI never accepts the stub fallback.
- `storlive.exe` imports `obs.dll`.
- Portable contains `obs.dll`, `libobs-d3d11.dll`, RTMP plugins and Windows capture/audio plugins.
- Portable contains `data/libobs` and plugin data.
- Both platform artifacts upload successfully.
- Tagged builds publish ZIP + DEB + SHA256 checksums only after both jobs pass.

Physical capture and platform credentials still depend on the user's hardware, drivers, permissions and RTMP account settings; CI validates the complete compiled/runtime package rather than external streaming accounts.
