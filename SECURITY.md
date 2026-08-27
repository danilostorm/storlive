# SECURITY

## Reporting

Do not publish stream keys, RTMP credentials, passwords or account tokens in issues or logs. Redact credentials before attaching screenshots or diagnostic output.

For ordinary non-sensitive bugs, use the repository issue tracker. For credential exposure, rotate/revoke the affected credential immediately at the streaming platform before doing anything else.

## Release integrity

Official tagged releases include `SHA256SUMS.txt`. Windows release CI rejects packages where `storlive.exe` is not linked to `obs.dll` or where mandatory capture/RTMP plugins are missing.
