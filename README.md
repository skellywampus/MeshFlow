# MeshFlow

**An experimental Meshtastic firmware build adapted for the Elecrow ThinkNode M9.**

The M9 combines a 320 × 240 color display, a physical QWERTY keyboard, a directional pad, LoRa radio, and optional GPS in a handheld device. I built MeshFlow to make Meshtastic comfortable to use on that hardware. I reshaped the on-device interface around the small screen and keyboard, and added M9-specific display and power-management changes.

MeshFlow is based on Meshtastic firmware. It keeps Meshtastic's device configuration, packet handling, LoRa communication, encryption, routing, and messaging as the foundation. It does not replace the Meshtastic network with a separate protocol or application.

> **Experimental beta:** Intended for M9 users who are comfortable testing community firmware and reporting issues. It is not an official Meshtastic release. Keep a known-good firmware image available before installing.

## How MeshFlow differs on the M9

| Area | Meshtastic foundation | MeshFlow for ThinkNode M9 |
| --- | --- | --- |
| Interface | Device UI designed to support Meshtastic's range of devices | WADAMESH-inspired visual styling and screen layouts adapted to the M9's 320 × 240 display |
| Navigation | Shared Device UI navigation and configuration flows | Home dashboard, launcher, and full-width primary screens arranged for the M9's physical keyboard and directional controls |
| Display | Standard display and LVGL behavior for supported boards | M9-specific LVGL rendering, refresh, and display power handling intended to keep the interface responsive while reducing unnecessary screen activity |
| Power | Meshtastic radio and device behavior | M9-specific CPU scaling and automatic light sleep while eligible, with wake sources and peripheral activity coordinated to keep normal mesh operation available |
| GPS | Meshtastic location configuration and scheduled position behavior | When GPS is configured off, the GPS button can request a one-time fix for up to five minutes. Press it again to cancel. A successful fix updates the local position without immediately broadcasting it; the saved GPS mode stays off. |

The interface changes are a device-specific adaptation of Meshtastic Device UI, not a wholesale rewrite of Meshtastic. The goal is to make existing Meshtastic functions easier to see and operate on the M9, while leaving the underlying mesh behavior familiar.

## Battery life and power management

During my hardware testing across several iterations, I observed roughly **1% battery use per hour with Wi-Fi and GPS off**. That is my observation from one M9 under those conditions, not a controlled battery test or a guaranteed runtime. Screen use, LoRa traffic, signal strength, notification sounds, temperature, battery condition, and enabled radios can all change consumption. Your results may differ; please share measurements with the device settings and test duration.

MeshFlow's M9 power work includes lower CPU frequency while the display is off, automatic light sleep when current activity permits it, and display suspension during screen-off periods. It is designed to leave Meshtastic networking available; it does not intentionally disable LoRa reception to save power. Wi-Fi or GPS activity can prevent some sleep opportunities, so the observed low-drain condition above had both turned off.

## Quality-of-life changes

- The home screen and launcher are arranged for the M9's compact display, with keyboard and directional-pad navigation in mind.
- Chats and message bubbles are kept aligned with the newest conversation. If you are already at the bottom, new messages and delivery-status changes follow the latest message; scrolling back to older messages should preserve your place.
- Failed outgoing messages are retried automatically before being marked failed. Delivery status remains visible in the conversation.
- Favorite nodes are grouped at the top of the node list while still respecting active filters.
- Ringtone volume can be set to off, low, medium, or high.
- A configured-off GPS can be started for one temporary position acquisition from the GPS button, canceled with another press, and then powered back down after a fix or timeout.
- Press Home twice to lock and turn off the display. Double-click Center to unlock. The first key used to wake the screen after an ordinary timeout is consumed as a wake action.

## Using MeshFlow on the M9

### Getting around

I designed the home dashboard and launcher around the M9's 320 × 240 screen. Use the directional pad to move the selection and Center or Enter to open an item. Home, Messages, Nodes, Channels, Maps, Settings, and status information use layouts adapted to the display. The home screen shows the clock, date, battery, Wi-Fi, and GPS status.

### Messages

Open **Messages** and select a conversation to view and send messages with the physical keyboard. MeshFlow shows delivery status on outgoing messages and opens conversations at their most recent messages. When you are already at the bottom, incoming messages and status updates should remain visible; you can scroll up to read earlier messages without being pulled back down.

I added automatic retries when the firmware reports a send failure: up to two retries follow the initial attempt, after which a persistent failure is shown. You can also retry an unconfirmed message manually. Delivery acknowledgments depend on the network and recipient; they are not a guarantee that a person has read the message.

### Nodes and position

Open **Nodes** and select a node to access its actions. The node menu provides direct messaging, sending your last known position, requesting position or node information, traceroute, showing the node on the map, favorite controls, and node details. Actions that need a position depend on a usable saved location; a last known position may be old. Favorite nodes appear first within the current filter, so a filter can still hide a favorite.

### GPS

When GPS is configured off, press the GPS button to request one fresh position. MeshFlow powers the receiver, searches for up to five minutes, then powers it down after a fix or timeout. Press the GPS button again to cancel. The saved GPS setting remains off, and a successful fix updates the local position without immediately broadcasting it. A fixed-position configuration is not overridden. Acquisition time depends on reception and the receiver's previous state.

### Display and power

Press **Home twice** to lock and turn off the display; double-click **Center** to unlock. For an ordinary screen timeout, the first key press wakes the display without also activating the selected control. MeshFlow is designed to keep the radio operating while the display is off.

To inspect power behavior, open **Settings & Tools → Tools → Power Statistics**. It reports available battery information, screen and CPU activity, and light-sleep statistics. Reported battery percentage and discharge rate are estimates, and an idle CPU at 80 MHz does not by itself mean the device is in light sleep.

### Ringtones

Use the ringtone volume setting to choose **Off**, **Low**, **Medium**, or **High**. Existing Meshtastic notification preferences still determine whether a message makes a sound.

## What to expect

MeshFlow is developed and built for the Elecrow ThinkNode M9 (`thinknode_m9-tft`). It is not a general-purpose build for other Meshtastic boards. Some interface and power behavior depends on the M9's display, keyboard controller, wiring, and peripherals.

I have built and installed the firmware on my M9 and used it on hardware, but each new change still needs focused testing. In particular, test one-time GPS acquisition and cancellation, message status visibility at the bottom of a conversation, keyboard wake, light-sleep behavior, and LoRa reception in your setup. Battery-life results depend on radio traffic, enabled peripherals, signal conditions, and use; no universal battery-life figure is promised.

## Installation caution

This is experimental firmware. Before installing, make sure you can restore a known-good ThinkNode M9 firmware and understand the recovery process. Back up device configuration and any information you need to preserve. A successful build alone does not prove that a feature works correctly on every M9.

## Beta firmware

The [M9 application image](release/firmware.bin), its [SHA-256 checksum](release/SHA256SUMS), and [beta release notes](RELEASE-NOTES.md) are included for evaluation. It was built for the `thinknode_m9-tft` target and flashed on my M9. This is an application image, not a factory image. Installation depends on the device's actual partition layout; do not use a full-flash erase to install it. The intended installation method is the Meshtastic Web Flasher with a locally uploaded update image; see the [installation guide](INSTALLATION.md). I have not yet tested the complete browser workflow on an M9.

## Source and build status

The `firmware/` and `device-ui/` directories contain snapshots of the source used in my local M9 build, including local modifications. They retain their upstream license files. The M9 configuration points to the included Device UI and uses the `thinknode_m9-tft` PlatformIO target.

**The source snapshot and its custom SDK have been built in an isolated environment.** The SDK configuration matched the recorded beta configuration. The resulting image differs from the flashed beta and has not been hardware tested; the first clean build also needed a PlatformIO version workaround. The upstream firmware submodule sources are included at their recorded revisions. See [SOURCE-NOTES.md](SOURCE-NOTES.md) for the captured revisions and limitations. The build is not yet bit-for-bit reproducible, and the new image has not been tested on hardware.

## Credits and licensing

MeshFlow builds on [Meshtastic](https://github.com/meshtastic/firmware) and takes visual inspiration from WADAMESH. See [attribution and license information](ATTRIBUTION.md) and the notices included with the source trees. MeshFlow is an independent project and is not affiliated with or endorsed by the Meshtastic or WADAMESH maintainers.
