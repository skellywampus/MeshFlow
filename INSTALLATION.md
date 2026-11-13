# Installing the M9 beta

The image in `release/firmware.bin` is an ESP32-S3 application image for the Elecrow ThinkNode M9, not a complete factory image. The build used for the beta was written at `0x10000` on an M9 whose existing application partition started there and was 4,063,232 bytes. Check the target device's existing partition table and active boot arrangement before using that offset: other layouts may differ.

Before installation, export any configuration and credentials you need to retain, and keep a known-good recovery image and its installation instructions available. The beta does not include a universal recovery image. An application-only write preserves data outside its write range; it does not replace a backup or guarantee compatibility with every existing installation.

Verify the downloaded image against `release/SHA256SUMS`. For example, on macOS or Linux, from the `release` directory:

```sh
shasum -a 256 -c SHA256SUMS
```

On a device whose partition layout and active app slot have been verified to match the development M9, an experienced user can use esptool's `write-flash` command to write only this image at `0x10000`. For that verified layout, the command has this form, replacing `PORT` with the M9's serial port:

```sh
esptool --chip esp32s3 --port PORT --baud 115200 write-flash 0x10000 firmware.bin
```

Run it from the `release` directory. Select the correct serial port explicitly. Do not use `erase-flash` or a factory image as part of this update. Keep the device connected until esptool reports that the write and its built-in verification have completed.

These are beta notes for experienced testers. A complete installation guide covering other operating systems, partition layouts, backups, and recovery is still being prepared.
