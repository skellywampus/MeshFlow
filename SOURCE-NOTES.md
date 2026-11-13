# Source and build notes

This directory is a snapshot assembled from the local MeshFlow workspace. It includes uncommitted modifications present in the working trees; commit IDs below identify their base revisions, not complete representations of the snapshot.

- Meshtastic firmware base: `64dd5b314af986d300b8f7c08c54f9b90ff52e6e`.
- Meshtastic Device UI base: `79dc3ff34f73c66ef4cbc6d5457eb2748b0ba815`.
- The beta firmware was built locally as the `thinknode_m9-tft` target with PlatformIO and ESP-IDF 5.5.5, using a locally prepared SDK and cached dependencies.
- The firmware snapshot does not include Git history. The `protobufs` and `meshtestic` directories contain exported source at the upstream gitlink revisions `85f3f4357946a52daa5310285476d2dffb137c7b` and `dcac7e5673005f4d8a2b1f0f6e06877b689d7519` respectively. Consult `firmware/.gitmodules` for upstream repositories.
- Absolute package-cache paths were removed from `firmware/platformio.ini`; the M9 target was adjusted to refer to the adjacent Device UI snapshot. The Desktop source snapshot built successfully for `thinknode_m9-tft` using the existing locally prepared SDK and newly resolved project dependencies. This was not an independent SDK build.
- The installed M9 application partition observed during development was 4,063,232 bytes at `0x10000`. Partition layouts can differ; inspect the target device before attempting an application-only flash.
- The recorded SDK configuration is in `sdk/esp32s3-sdkconfig`; the SDK libraries themselves are not included. A separate SDK rebuild from these settings was subsequently completed as described below.

## Desktop build check

The Desktop snapshot built successfully for `thinknode_m9-tft` using PlatformIO and the existing custom SDK. The observed M9 application partition is 4,063,232 bytes; check the target device before flashing.

## Rebuilding the ESP32 SDK

The original SDK build log identifies pioarduino platform `55.03.312-1`, Arduino-ESP32 `3.3.12`, ESP-IDF `5.5.5`, and framework libraries `5.5.5+sha.b774170ff46`. The platform URL is pinned in `firmware/variants/esp32/esp32-common.ini`. The M9 target's `custom_sdkconfig` section selects power management, tickless idle, sleep callbacks, and Bluetooth settings; pioarduino's Arduino builder detects custom SDK settings and invokes its ESP-IDF library compilation path when the installed framework libraries do not match them. `sdk/esp32s3-sdkconfig` records the configuration from the SDK used for the existing beta.

To attempt a clean build without changing an existing PlatformIO installation, use an isolated `PLATFORMIO_CORE_DIR` and run `pio run -e thinknode_m9-tft` from `firmware/`. The first build may download packages and rebuild ESP-IDF libraries. Compare its generated ESP32-S3 SDK configuration to the recorded file, especially `CONFIG_PM_ENABLE`, `CONFIG_FREERTOS_USE_TICKLESS_IDLE`, and `CONFIG_PM_LIGHT_SLEEP_CALLBACKS`. A successful compile alone does not verify radio wake or automatic light sleep.

### Isolated SDK rebuild result

On macOS, an isolated PlatformIO core rebuilt the ESP32-S3 SDK from the staged source. Its generated `sdkconfig` matched `sdk/esp32s3-sdkconfig` byte for byte. The subsequent M9 application build succeeded. A successful build does not verify the firmware’s behavior on physical hardware.

The first attempt required a PlatformIO workaround: the parent `pio` was Core 6.2.0, while the automatically created nested `penv/bin/pio` reported 6.2.0.post1 and was rejected by the pinned platform as incompatible. After the SDK libraries were generated, the nested executable in the isolated core was redirected to the working parent `pio`, and rerunning the build succeeded. This workaround affected only the disposable isolated core. The one-command fresh setup therefore is not yet verified without intervention.

For this macOS verification, after the first build failed, the isolated core's nested `pio` was redirected to the same Core 6.2.0 executable used for the outer build. The following illustrates that workaround after the isolated core has been created; it changes only the selected core directory:

```sh
# PLATFORMIO_CORE_DIR must point to your isolated core.
outer_pio="$(command -v pio)"
mv "$PLATFORMIO_CORE_DIR/penv/bin/pio" "$PLATFORMIO_CORE_DIR/penv/bin/pio.original"
printf '#!/bin/sh\nexec "%s" "$@"\n' "$outer_pio" > "$PLATFORMIO_CORE_DIR/penv/bin/pio"
chmod +x "$PLATFORMIO_CORE_DIR/penv/bin/pio"
pio run -e thinknode_m9-tft
```

Run the final command from `firmware/`. Do not replace a `pio` in a shared PlatformIO core; use an isolated `PLATFORMIO_CORE_DIR`. The wrapper approach has only been exercised on macOS.
