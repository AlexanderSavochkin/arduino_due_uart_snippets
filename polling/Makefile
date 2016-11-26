CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
AR=arm-none-eabi-ar
LIB=../lib
LINKER_SCRIPTS=../sam/linker_scripts/gcc

MAIN=uart_write_polling.c
SOURCES=$(MAIN) ../syscalls.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=main

CFLAGS=-c -g -Os -w -ffunction-sections -fdata-sections -nostdlib \
	-I/usr/lib/arm-none-eabi/include/ \
        -I../include \
        -I../sam/libsam \
        -I../sam/CMSIS/CMSIS/Include/  \
        -I../sam/CMSIS/Device/ATMEL/ \
	--param max-inline-insns-single=500 -fno-rtti -fno-exceptions \
	-Dprintf=iprintf -mcpu=cortex-m3 -DF_CPU=84000000L \
	-DARDUINO=156 -DARDUINO_SAM_DUE -DARDUINO_ARCH_SAM -D__SAM3X8E__ \
	-mthumb -DUSB_VID=0x2341 -DUSB_PID=0x003e -DUSBCON \
	-DUSB_MANUFACTURER="Unknown" -DUSB_PRODUCT="Arduino Due"

LDFLAGS=-Os -Wl,--gc-sections -mcpu=cortex-m3 -T$(LINKER_SCRIPTS)/flash.ld \
	-Wl,-Map,$(MAIN).map -o $(MAIN).elf -lm -lgcc -mthumb \
	-Wl,--cref -Wl,--check-sections -Wl,--gc-sections \
	-Wl,--entry=Reset_Handler -Wl,--unresolved-symbols=report-all \
	-Wl,--warn-common -Wl,--warn-section-align \
	-Wl,--warn-unresolved-symbols

all: $(SOURCES) $(EXECUTABLE)

libsam:
	cd ../sam/libsam/build_gcc/ && make arduino_due_x

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE): libsam $(OBJECTS)
	$(CC) $(LDFLAGS) -Wl,--start-group $(OBJECTS) $(LIB)/libsam_sam3x8e_gcc_rel.a -Wl,--end-group
	$(OBJCOPY) -O binary $(MAIN).elf $(MAIN).bin

prog: $(EXECUTABLE)
	stty -F /dev/ttyACM0 1200
	bossac -i -d --port=ttyACM0 -U false -e -w -v -b $(MAIN).bin -R

clean:
	rm -f test *.elf *.a *.o *.map *.bin
	cd ../sam/libsam/build_gcc/ && make clean
