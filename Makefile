ifneq ($(EMULATOR),1)
OBJS += startup.o
endif

OBJS += at88sc0104.o
OBJS += cryptomem.o
OBJS += buttons.o
OBJS += layout.o
OBJS += oled.o
OBJS += rng.o
OBJS += serialno.o

ifneq ($(EMULATOR),1)
OBJS += setup.o
endif

OBJS += util.o
OBJS += memory.o
OBJS += supervise.o

ifneq ($(EMULATOR),1)
OBJS += timer.o
endif

OBJS += gen/bitmaps.o
OBJS += gen/fonts.o

ALL: firmware

ifneq ($(EMULATOR),1)
$(OBJS): libopencm3
endif

libtrezor.a: $(OBJS)
	$(AR) rcs libtrezor.a $(OBJS)

include Makefile.include

.PHONY: ALL vendor bootloader firmware nanopb

vendor:
	git submodule update --init

nanopb:
	$(MAKE) -C vendor/nanopb/generator/proto

firmware_protob: nanopb
	$(MAKE) -C firmware/protob

firmware: firmware_protob bootloader libtrezor.a
	$(MAKE) -C firmware sign

emulator/libemulator.a:
	EMULATOR=1 $(MAKE) -C emulator

emulator: firmware_protob libtrezor.a emulator/libemulator.a
	EMULATOR=1 $(MAKE) -C firmware

bootloader/bootloader.bin: libtrezor.a
	$(MAKE) -C bootloader align

bootloader: bootloader/bootloader.bin

mostlyclean: clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C firmware clean
	$(MAKE) -C emulator clean

allclean: mostlyclean
	$(MAKE) -C vendor/libopencm3 clean

libopencm3:
	FP_FLAGS="-mfloat-abi=soft" V=1 $(MAKE) -C vendor/libopencm3

