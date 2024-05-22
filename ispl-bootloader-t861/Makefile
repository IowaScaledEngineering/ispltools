#*************************************************************************
#Title:    SoundBytes Plus Makefile
#Authors:  Michael Petersen <railfan@drgw.net>
#          Nathan Holmes <maverick@drgw.net>
#File:     CKT-DINGDONG Makefile
#License:  GNU General Public License v3
#
#LICENSE:
#    Copyright (C) 2023 Nathan Holmes and Michael Petersen
#
#    Derived from example WinAVR project Makefile
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 3 of the License, or
#    any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#    
#    You should have received a copy of the GNU General Public License along 
#    with this program. If not, see http://www.gnu.org/licenses/
#    
#*************************************************************************

BASE_NAME = bootloader

DEVICE  = attiny861
F_CPU   = 8000000  # Hz
FUSE_L  = 0xE2
FUSE_H  = 0xD6
FUSE_E  = 0xFE


# BOOTLOADER_ADDRESS must be a multiple of attiny861 page size (64) and in hex (despite no 0x)
BOOTLOADER_ADDRESS = 1800

#PROGRAMMER_TYPE=avrispmkii
PROGRAMMER_TYPE=iseavrprog
#PROGRAMMER_TYPE=dragon_isp
PROGRAMMER_PORT=usb

DEFINES = -DBOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS)

SRCS = bootloader.c spiflash.c

AVRDUDE = avrdude -P $(PROGRAMMER_PORT) -c $(PROGRAMMER_TYPE) -p $(DEVICE) -B8 -F
AVRDUDE_SLOW = avrdude -P $(PROGRAMMER_PORT) -c $(PROGRAMMER_TYPE) -p $(DEVICE) -B32

OBJS = ${SRCS:.c=.o}
INCLUDES = -I. 
CFLAGS  = $(INCLUDES) -Wall -Os -std=gnu99 -ffunction-sections -fpack-struct -fshort-enums -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions
LDFLAGS = -Wl,--relax,--gc-sections

COMPILE = avr-gcc $(DEFINES) -DF_CPU=$(F_CPU) $(CFLAGS) $(LDFLAGS) -mmcu=$(DEVICE)

help:
	@echo "make hex ....... build $(BASE_NAME).hex"
	@echo "make flash ..... flash the firmware"
	@echo "make fuse ...... flash the fuses"
	@echo "make program ... flash fuses and firmware"
	@echo "make firmware .. flash firmware from file"
	@echo "make read ...... read the fuses"
	@echo "make size ...... memory usage"
	@echo "make clean ..... delete objects and hex file"
	@echo "make release.... produce release tarball"
	@echo "make terminal... open up avrdude terminal"

hex: boot-img.hex

program: fuse flash

terminal:
	$(AVRDUDE_SLOW) -t

loader.elf: $(OBJS)
	$(COMPILE) -o $@ $(OBJS) $(LDFLAGS) -Wl,--section-start=.text=$(BOOTLOADER_ADDRESS)
	avr-size --format=avr --mcu=$(DEVICE) $@
	avr-objdump -x -D -S -z $@ > $@.lss

loader.hex: loader.elf
	rm -f loader.hex
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@

jump.elf:	jump.asm
	avr-as -mmcu=$(DEVICE) --defsym BOOTLOADER_ADDRESS=0x$(BOOTLOADER_ADDRESS) -o $@ $<

jump.hex:	jump.elf
	rm -f $@ $@.tmp jump.hex
	avr-objcopy -j .text -j .data -O ihex $< $@.tmp
	head -1 $@.tmp > $@
	rm -f $@.tmp

boot-img.hex: jump.hex loader.hex
	cat jump.hex loader.hex > $@
	avr-objdump -mavr -D $@ > $@.lss


# rule for programming fuse bits:
fuse:
	@[ "$(FUSE_H)" != "" -a "$(FUSE_L)" != "" -a "$(FUSE_E)" != "" ] || \
		{ echo "*** Invalid Fuse values."; exit 1; }
	$(AVRDUDE_SLOW) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m -U efuse:w:$(FUSE_E):m

read:
	$(AVRDUDE) -v

size:
	avr-size -C --mcu=$(DEVICE) $(BASE_NAME).elf



# rule for uploading firmware:
flash: hex
	$(AVRDUDE) -U flash:w:boot-img.hex:i

firmware:
	$(AVRDUDE) -U flash:w:$(HEX):i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f boot.hex jump.hex bootloader.hex $(BASE_NAME).lst $(BASE_NAME).obj $(BASE_NAME).cof $(BASE_NAME).list $(BASE_NAME).map $(BASE_NAME).eep.hex *.elf $(BASE_NAME).s $(OBJS) *.o *.tgz *~

# Generic rule for compiling C files:
.c.o: $(INCS)
	$(COMPILE) -c $< -o $@

# file targets:

$(BASE_NAME).elf: $(OBJS)
	$(COMPILE) -o $(BASE_NAME).elf $(OBJS)

$(BASE_NAME).hex: $(BASE_NAME).elf
	rm -f $(BASE_NAME).hex $(BASE_NAME).eep.hex
	avr-objcopy -j .text -j .data -O ihex $(BASE_NAME).elf $(BASE_NAME).hex
	avr-size $(BASE_NAME).hex

# debugging targets:

disasm:	$(BASE_NAME).elf
	avr-objdump -d $(BASE_NAME).elf




