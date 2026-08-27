# Legacy audit

The previous StorLive material supplied for migration was treated only as a behavioral/reference source. The desktop implementation in this repository is a clean C++/Qt/libobs codebase focused on multi-live.

## Migrated product intent

- Multiple simultaneous streaming destinations.
- Platform presets plus custom RTMP.
- A small operator-focused interface.

## Intentionally not migrated

- Web/server-specific code and deployment files.
- Credentials, stream keys, tokens or account secrets.
- Obsolete media/cache/build artifacts.
- Features unrelated to desktop capture + multi-live.

## Desktop replacement

StorLive now owns capture, scene composition, preview, encoder selection and RTMP outputs directly through libobs. Windows ships a controlled portable libobs runtime; Linux uses system libobs/OBS plugins.
