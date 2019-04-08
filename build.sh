#!/bin/bash
set -e

IMAGE=trezor-mcu-build

BOOTLOADER_TAG=${1:-master}
FIRMWARE_TAG=${2:-master}

BOOTLOADER_BINFILE=build/bootloader-$BOOTLOADER_TAG.bin
BOOTLOADER_ELFFILE=build/bootloader-$BOOTLOADER_TAG.elf

FIRMWARE_BINFILE=build/safet-$FIRMWARE_TAG.bin
FIRMWARE_ELFFILE=build/safet-$FIRMWARE_TAG.elf

docker build -t $IMAGE .
docker run -t -v $(pwd)/build:/build:z $IMAGE /bin/sh -c "\
	cd /tmp && \
	git clone https://github.com/syscoin/safe-t-mcu.git safe-t-mcu-bl && \
	cd safe-t-mcu-bl && \
	git checkout $BOOTLOADER_TAG && \
	git submodule update --init --recursive && \
	make bootloader MEMORY_PROTECT=1 && \
	make -C bootloader align && \
	cp bootloader/bootloader.bin /$BOOTLOADER_BINFILE && \
	cp bootloader/bootloader.elf /$BOOTLOADER_ELFFILE && \
	cd /tmp && \
	git clone https://github.com/syscoin/safe-t-mcu.git archos-safe-t-mcu-fw && \
	cd archos-safe-t-mcu-fw && \
	git checkout $FIRMWARE_TAG && \
	git submodule update --init --recursive && \
	make firmware MEMORY_PROTECT=1 UPDATE_BOOTLOADER=1 && \
	cp /tmp/safe-t-mcu-bl/bootloader/bootloader.bin bootloader/bootloader.bin && \
	make -C firmware sign MEMORY_PROTECT=1 UPDATE_BOOTLOADER=1 && \
	cp firmware/trezor.bin /$FIRMWARE_BINFILE && \
	cp firmware/trezor.elf /$FIRMWARE_ELFFILE
	"

/usr/bin/env python -c "
from __future__ import print_function
import hashlib
import sys
for arg in sys.argv[1:]:
  (fn, max_size) = arg.split(':')
  data = open(fn, 'rb').read()
  print('\n\n')
  print('Filename    :', fn)
  print('Fingerprint :', hashlib.sha256(hashlib.sha256(data).digest()).hexdigest())
  print('Size        : %d bytes (out of %d maximum)' % (len(data), int(max_size, 10)))
" $BOOTLOADER_BINFILE:32768 $FIRMWARE_BINFILE:491520
