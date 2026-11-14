# MeshFlow M9 beta update

## Latest build — 14 November 2026

This update fixes the Home screen’s online-node count remaining stale after startup until the Nodes list was opened. The count now refreshes from the node store at startup, on node changes, and as nodes age offline, while list presentation work remains deferred until the Nodes view is open.

- Target: `thinknode_m9-tft` (Elecrow ThinkNode M9).
- Application size: 4,020,448 bytes.
- SHA-256: `458239210078860bf440d2fc7a8229d4e817e278bc56eb8ba2f0005443a5a5df`.
- Flash image: `release/firmware.bin`; checksum: `release/SHA256SUMS`.
- Build: passed. The NodeStore regression suite passed 30 cases and 193 assertions.
- This image was flashed to the development M9. Longer-term physical behavior remains subject to testing.

## Included M9 changes

The beta includes the M9 interface and navigation updates, message and node-list layout fixes, theme readability corrections, Power Statistics, one-time GPS acquisition, configurable ringtone volume, automatic send retries, keyboard wake behavior, and M9 display/power-management work. The experimental unread-message keyboard blink was removed; keyboard wake protection and immediate keyboard-light shutoff on manual screen sleep remain.

Physical radio, power, display, GPS, and keyboard behavior can vary by device and use. See [INSTALLATION.md](INSTALLATION.md) for installation and recovery guidance.
