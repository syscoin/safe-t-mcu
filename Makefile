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

libtrezor.a: $(OBJS)
	$(AR) rcs libtrezor.a $(OBJS)

include Makefile.include

.PHONY: ALL vendor bootloader firmware

vendor:
	git submodule update --init

firmware: libopencm3 bootloader
	$(MAKE) -C vendor/nanopb/generator/proto
	$(MAKE) -C firmware/protob
	$(MAKE) libtrezor.a
	$(MAKE) -C firmware sign

bootloader: libopencm3
	$(MAKE) libtrezor.a
	$(MAKE) -C bootloader align

mostlyclean: clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C firmware clean

allclean: mostlyclean
	$(MAKE) -C vendor/libopencm3 clean

libopencm3:
	FP_FLAGS="-mfloat-abi=soft" V=1 $(MAKE) -C vendor/libopencm3

