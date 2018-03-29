ifneq ($(EMULATOR),1)
OBJS += startup.o
endif

OBJS += at88sc0104.o
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

firmware:
	FP_FLAGS="-mfloat-abi=soft" V=1 make -C vendor/libopencm3
	make -C vendor/nanopb/generator/proto
	make -C firmware/protob
	make libtrezor.a
	make -C firmware sign

bootloader:
	FP_FLAGS="-mfloat-abi=soft" V=1 make -C vendor/libopencm3
	make libtrezor.a
	make -C bootloader align

allclean: clean
	make -C vendor/libopencm3 clean
	make -C bootloader clean

