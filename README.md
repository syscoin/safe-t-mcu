# Safe-T mini Bootloader and Firmware

https://safe-t.io/

The Safe-T mini firmware is based on a fork of the proven software by [Trezor](https://github.com/trezor/trezor-mcu). The Safe-T mini uses a CrypoMemory ([AT88SC0104CA](https://www.microchip.com/wwwproducts/en/AT88SC0104CA)) chip to protect the valuable information stored on the device even better. 

## How to build the Safe-T mini bootloader and firmware?

1. [Install Docker](https://docs.docker.com/engine/installation/)
2. `git clone https://github.com/archos-safe-t/safe-t-mcu.git`
3. `cd safe-t-mcu`
4. `./build.sh BOOTLOADER_TAG FIRMWARE_TAG` (where BOOTLOADER_TAG is bl1.5.1 and FIRMWARE_TAG is v1.0.7 for example, if left blank the script builds latest commit in master branch)

This creates the files `build/bootloader-BOOTLOADER_TAG.bin` and `build/safet-FIRMWARE_TAG.bin` and prints their fingerprints and sizes at the end of the build log.

Note however that you will not be able to run a firmware built this way using the CRYPTOMEM on a regular device, as the bootloader will drop permissions for unsigned firmware that are needed.

## How to build Safe-T mini emulator for Linux?

1. [Install Docker](https://docs.docker.com/engine/installation/)
2. `git clone https://github.com/archos-safe-t/safe-t-mcu.git`
3. `cd safe-t-mcu`
4. `./build-emulator.sh TAG` (where TAG is v1.0.7 for example, if left blank the script builds latest commit in master branch)

This creates binary file `build/trezor-emulator-TAG`, which can be run and works as a trezor emulator. (Use `TREZOR_OLED_SCALE` env. variable to make screen bigger.)

The emulator uses the "left" and "right" arrow keys from the keyboard to emulate the device buttons.

## How to get fingerprint of firmware signed and distributed by Archos?

1. Pick version of firmware binary listed on https://storage.googleapis.com/safe-t-software/safe-t-mini/latest_version.txt
2. Download it: `wget https://storage.googleapis.com/safe-t-software/safe-t-mini/firmware/1.0.7/safe-t-mini.bin`
3. Compute fingerprint: `tail -c +257 safe-t-mini.bin | sha256sum`

Step 3 should produce the same sha256 fingerprint as your local build (for the same version tag). Firmware has a special header (of length 256 bytes) holding signatures themselves, which must be skipped while calculating the fingerprint, that's why tail command has to be used.

## How to install custom built firmware?

**WARNING: This will erase the recovery seed stored on the device! You should never do this on Safe-T mini that contains coins!**

1. Install python-safet `pip3 install safet` ([more info](https://github.com/archos-safe-t/python-safet))
2. `safetctl firmware_update -f build/safet-TAG.bin`

## Building for development

If you want to build device firmware, make sure you have the
[GNU ARM Embedded toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads) installed.

* If you want to build the emulator instead of the firmware, run `export EMULATOR=1`
* If you want to build with the debug link, run `export DEBUG_LINK=1`. Use this if you want to run the device tests.
* When you change these variables, use `script/setup` to clean the repository

1. To initialize the repository, run `script/setup`
2. To build the firmware or emulator, run `script/cibuild`

If you are building device firmware, the firmware will be in `firmware/trezor.bin`.

You can launch the emulator using `firmware/trezor.elf`. To use `safetctl` with the emulator, use
`safetctl -p udp` (for example, `safetctl -p udp get_features`).
