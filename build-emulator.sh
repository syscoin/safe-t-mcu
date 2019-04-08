#!/bin/bash
set -e

IMAGE=safet-mcu-build-emulator64
TAG=${1:-master}
ELFFILE=build/safet-emulator64-$TAG

docker build -f Dockerfile.emulator -t $IMAGE .
docker run -t -v $(pwd)/build:/build:z $IMAGE /bin/sh -c "\
	git clone https://github.com/syscoin/safe-t-mcu.git && \
	cd safe-t-mcu && \
	git checkout $TAG && \
	git submodule update --init && \
	EMULATOR=1 make emulator && \
	cp firmware/trezor.elf /$ELFFILE"
