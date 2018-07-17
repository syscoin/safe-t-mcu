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

libtrezor.a: $(OBJS)
	$(AR) rcs libtrezor.a $(OBJS)

include Makefile.include

.PHONY: vendor bootloader firmware

vendor:
	git submodule update --init

firmware: libopencm3
	$(MAKE) -C vendor/nanopb/generator/proto
	$(MAKE) -C firmware/protob
	$(MAKE) libtrezor.a
	$(MAKE) -C firmware sign

bootloader: libopencm3
	$(MAKE) libtrezor.a
	$(MAKE) -C bootloader align

allclean: clean
	$(MAKE) -C vendor/libopencm3 clean
	$(MAKE) -C bootloader clean

libopencm3:
	FP_FLAGS="-mfloat-abi=soft" V=1 $(MAKE) -C vendor/libopencm3

