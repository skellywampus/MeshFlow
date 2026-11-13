# MeshFlow M9 experimental beta

The included app-only image was flashed on the developer's ThinkNode M9 and verified by the flash tool. It includes the redesigned M9 interface, display and power-management changes, one-time GPS acquisition, and message status scrolling changes described in the README. The latest GPS and message-scrolling changes still need focused hardware confirmation.

- Target: `thinknode_m9-tft` (Elecrow ThinkNode M9).
- Application size: 4,025,952 bytes.
- SHA-256: `82c8fe325dd6c5346208140b99e2823076fbb103a1567cb1e0733ce36c7a967f`.
- Flash image: `release/firmware.bin`; checksum file: `release/SHA256SUMS`.
- Installation constraints and recovery preparation: see `INSTALLATION.md`.
- Source snapshots, SDK details, and reproducibility limits: see `SOURCE-NOTES.md`.

Subsequent source builds succeeded using the existing SDK and an independently rebuilt SDK, but generated different binaries and have not been flashed or hardware tested. The independent run required a PlatformIO version workaround; see `SOURCE-NOTES.md`. The included image is the earlier flashed beta, not that later build.
