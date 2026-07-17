# Third-party dependencies

| Dependency | Version / commit | License | Purpose |
|---|---|---|---|
| libspng | 0.7.4 / `fb768002d4288590083a476af628e51c3f1d47cd` | BSD-2-Clause | Bounded PNG parsing and encoding |
| zlib | 1.3.2 / `da607da739fa6047df13e66a2af6b8bec7c2a498` | Zlib | DEFLATE implementation used by libspng |
| ONNX Runtime | 1.26.0 / official Windows x64 CPU archive | MIT | Governed local CPU model inference |

CMake downloads immutable commit archives and verifies SHA-256 before
extraction. Both are built statically; their tests, examples, installers, and
shared libraries are disabled. Runtime code performs no dependency download or
network access. License texts remain in each fetched source tree/build cache;
binary/source distributions must reproduce the required notices.

The ONNX Runtime archive is independently locked to SHA-256
`6ebe99b5564bf4d029b6e93eac9ff423682b6212eade769e9ca3f685eaf500b4`.
Its runtime DLL is staged beside participating executables. Binary releases
must retain ONNX Runtime's license and complete upstream third-party notices.
