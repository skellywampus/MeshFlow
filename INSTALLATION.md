# Installing the M9 beta

MeshFlow is an experimental firmware update for the **Elecrow ThinkNode M9**. The release's `firmware.bin` is an application image; it is not a factory image or a recovery image for an erased device.

## Meshtastic Web Flasher

The [Meshtastic Web Flasher](https://flasher.meshtastic.org/) accepts a locally uploaded ESP32 update image. Its current source writes an uploaded update at `0x10000` without erasing the entire flash—the method used to install this image on my M9. **I have not yet tested the complete browser workflow on an M9**, so treat these steps as beta instructions. The flasher's appearance or behavior may change.

Before starting, back up configuration and credentials you need to keep, and have a known-good M9 recovery image and instructions available. This update relies on a compatible Meshtastic bootloader and partition layout already being present. On my M9, the application partition starts at `0x10000` and is 4,063,232 bytes. Other layouts or active boot slots may differ; do not proceed if you cannot confirm compatibility.

1. Download **`firmware.bin`** from the [MeshFlow beta release](https://github.com/skellywampus/MeshFlow/releases/tag/v0.1.1-beta). Do not rename it with a `.factory.bin` suffix.
2. Connect the ThinkNode M9 to the computer using a USB data cable.
3. Open the [Meshtastic Web Flasher](https://flasher.meshtastic.org/) in a browser that supports Web Serial, such as Chrome or Edge.
4. Select **ThinkNode M9**, then use the upload icon under Firmware to select the downloaded `firmware.bin`.
5. Use the normal **Update** option. Do **not** select **Full Erase and Install** or any erase-flash option. If the flasher only offers a full erase, stop.
6. Select the M9's serial port when prompted and keep it connected until the flasher reports completion.

A normal application-only update is intended to preserve data outside the written application region, but it does not replace a backup or guarantee compatibility with every M9 configuration. The included binary alone cannot restore an erased bootloader or partition table.

## Verify the download

The release also includes `SHA256SUMS`. On macOS or Linux, with both downloaded files in the same directory, run:

```sh
shasum -a 256 -c SHA256SUMS
```

## Alternative: esptool

For an M9 whose partition layout and active app slot have been verified to match the development device, an experienced user can write the application at `0x10000` with esptool. From the directory containing `firmware.bin`, replace `PORT` with the M9's serial port:

```sh
esptool --chip esp32s3 --port PORT --baud 115200 write-flash 0x10000 firmware.bin
```

Do not use `erase-flash` for this update. Keep the device connected until esptool reports that the write and its built-in verification have completed.
