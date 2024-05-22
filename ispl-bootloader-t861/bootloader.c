/*************************************************************************
Title:    SoundBytes Plus Bootloader
Authors:  Michael Petersen <railfan@drgw.net>
          Nathan D. Holmes <maverick@drgw.net>
File:     $Id: $
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2023 Michael Petersen, Nathan Holmes, with portions from 
     David Johson-Davies under a Creative Commons Attribution 4.0 license

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include "spiflash.h"
#include "util/delay.h"

#define PAGE_SZ_BYTES    64

#define ISPL_HEADER_IDENTIFIER_ADDR   0x00000000
#define ISPL_HEADER_IDENTIFIER_LEN    4
#define ISPL_HEADER_IDENTIFIER        "ISPL"

#define ISPL_MANIFEST_BASE_ADDR       0x00000008

#define ISPL_MANIFEST_REC_ADDR_OFFSET 0
#define ISPL_MANIFEST_REC_N_OFFSET    4
#define ISPL_MANIFEST_REC_S_OFFSET    8
#define ISPL_MANIFEST_REC_CRC_OFFSET  10

#define ISPL_TABLE_MANIFEST           0
#define ISPL_TABLE_PROGRAM            1
#define ISPL_TABLE_AUDIO              2

#define EEPROM_PROG_CRC32_BASE      508

typedef struct
{
	uint32_t baseAddr;
	uint32_t n;
	uint16_t s;
} ISPLTable;

bool isplTableLoad(ISPLTable* t, uint8_t tableNum)
{
	uint32_t n;
	uint32_t s;

	n = spiflashReadU32(ISPL_MANIFEST_BASE_ADDR + ISPL_MANIFEST_REC_N_OFFSET);
	s = spiflashReadU16(ISPL_MANIFEST_BASE_ADDR + ISPL_MANIFEST_REC_S_OFFSET);

	t->baseAddr = 0;
	t->n = 0;
	t->s = 0;

	if (tableNum >= n)
		return false;
	
	t->baseAddr = spiflashReadU32(ISPL_MANIFEST_BASE_ADDR + tableNum * s + ISPL_MANIFEST_REC_ADDR_OFFSET);
	t->n = spiflashReadU32(ISPL_MANIFEST_BASE_ADDR + tableNum * s + ISPL_MANIFEST_REC_N_OFFSET);
	t->s = spiflashReadU16(ISPL_MANIFEST_BASE_ADDR + tableNum * s + ISPL_MANIFEST_REC_S_OFFSET);

	return true;
}


bool isplInitialize(ISPLTable* isplProgramTable)
{
	uint8_t buffer[8];
	spiflashReadBlock(ISPL_HEADER_IDENTIFIER_ADDR, ISPL_HEADER_IDENTIFIER_LEN, buffer);
	if (0 != memcmp(buffer, ISPL_HEADER_IDENTIFIER, ISPL_HEADER_IDENTIFIER_LEN))
		return false;  // Header doesn't contain correct starting record

	return isplTableLoad(isplProgramTable, ISPL_TABLE_PROGRAM);
}

void beep()
{
	PORTA |= 0b10000000;  // Enable amplifier

	while(1)
	{
		for(uint16_t i=0 ; i < 150; i++)
		{
			PORTB ^= 0b00001000;

			switch((PINA & 0x30)>>4)
			{
				case 0x03:
					_delay_us(250);
				case 0x02:
					_delay_us(250);
				case 0x01:
					_delay_us(250);
				case 0x00:
					_delay_us(500);
					break;
			}
		}

		_delay_ms(1000);
	}
}


int main ( void )
{
	// Deal with watchdog first thing
	MCUSR = 0;	// Clear reset status
	wdt_disable();
	cli();

	// Let's just get the outputs in a sane state, because we need to talk to the SPI flash
	//  and probably keep the amplifier dead so it doesn't make weird noises

	// PORT A
	//  PA7 - Output - /SHUTDOWN to amplifier
	//  PA6 - Output - No Connection
	//  PA5 - Input  - /EN2x (enable pullup)
	//  PA4 - Input  - /EN1x (enable pullup)	
	//  PA3 - Output - /CS to flash
	//  PA2 - Output - CLK to flash
	//  PA1 - Output - MOSI to flash
	//  PA0 - Input - MISO to flash (enable pullup)

	// PORT B
	//  PB7 - n/a    - /RESET (not I/O pin)
	//  PB6 - Output - No Connection
	//  PB5 - Output - No Connection
	//  PB4 - Output - No Connection
	//  PB3 - Output - Audio PWM Output
	//  PB2 - Output - (AVR programming SPI)
	//  PB1 - Output - (AVR programming SPI)
	//  PB0 - Output - (AVR programming SPI)
	

	PORTA = 0b00111001;
	DDRA  = 0b11001110;

	PORTB = 0b00000000; 	// Just make everything low
	DDRB  = 0b11111111;     // And set it as an output

	// Let power stabilize a bit
	_delay_ms(100);

	spiSetup();
	spiflashReset();

	ISPLTable isplProgramTable;

	if (true == isplInitialize(&isplProgramTable))
	{
		// At this point, we have a valid ISPL program image
		bool crcMatch = false;
		uint32_t crc32_eeprom = 0;
		uint32_t crc32_spiflash = 0;

		// Check if the CRC is the same as the one we have in eeprom
		for(uint8_t i=0; !crcMatch && i<5; i++)
		{
			crc32_eeprom = eeprom_read_dword((const uint32_t*)EEPROM_PROG_CRC32_BASE);
			crc32_spiflash = spiflashReadU32(isplProgramTable.baseAddr + isplProgramTable.n - 4);

			if (crc32_eeprom == crc32_spiflash)
				crcMatch = true;
			else
				_delay_ms(100);
		}


		// If the program CRC isn't the same as what we have in flash, reflash the part
		if (!crcMatch)
		{
			bool programWriteSuccessful = true; // Negated if one of the pages fails to verify

			// The actual program size is 4 bytes less than reported because of the CRC on the end
			eeprom_write_dword((uint32_t*)EEPROM_PROG_CRC32_BASE, 0xFFFFFFFF);
			eeprom_busy_wait();

			// Full program space erase (up to bootloader)
			uint16_t progmemAddr = 0;
			for (progmemAddr = PAGE_SZ_BYTES; progmemAddr < BOOTLOADER_ADDRESS; progmemAddr += PAGE_SZ_BYTES)
				boot_page_erase(progmemAddr);

			uint8_t pageBuffer[PAGE_SZ_BYTES];
			uint16_t bytesLeftToWrite = isplProgramTable.n - 4; // Don't write the CRC
			progmemAddr = 0;
			uint16_t appResetRJMP = 0xFFFF;

			while (bytesLeftToWrite > 0 && progmemAddr < (BOOTLOADER_ADDRESS-PAGE_SZ_BYTES) && programWriteSuccessful)
			{
				uint8_t thisPageSz = min(PAGE_SZ_BYTES, bytesLeftToWrite); 

				boot_page_erase_safe(progmemAddr);

				spiflashReadBlock(isplProgramTable.baseAddr + progmemAddr, thisPageSz, pageBuffer);

				// If we're not getting a full page, fill with 0xFF
				if (thisPageSz < PAGE_SZ_BYTES)
					memset(pageBuffer + thisPageSz, 0xFF, PAGE_SZ_BYTES - thisPageSz);

				if (0 == progmemAddr)
				{
					// Save off the application's rjmp reset vector
					uint16_t appResetAddr = 0x0FFF & ((uint16_t)pageBuffer[0] | ((uint16_t)pageBuffer[1]<<8));
					appResetRJMP = 0xCFFF & ((appResetAddr - (BOOTLOADER_ADDRESS / 2) + 1) | 0xC000);

					// Calculate the bootloader's reset vector rjmp and put it in instead
					uint16_t bootloaderResetRJMP = 0xC000 | (((uint16_t)BOOTLOADER_ADDRESS / 2) - 1);
					pageBuffer[0] = (uint8_t)bootloaderResetRJMP;
					pageBuffer[1] = (uint8_t)(bootloaderResetRJMP>>8);
				}

				for(uint8_t i=0; i<PAGE_SZ_BYTES; i+=2)
				{
					uint16_t w = (uint16_t)pageBuffer[i] | ((uint16_t)pageBuffer[i+1]<<8);
					boot_page_fill_safe(progmemAddr + i, w);
				}

				boot_page_write_safe(progmemAddr);

				// Wait for write completion before verifying
				boot_spm_busy_wait();

				for(uint8_t i=0; programWriteSuccessful && i<PAGE_SZ_BYTES; i+=1)
				{
					if (pageBuffer[i] != pgm_read_byte(progmemAddr + i))
						programWriteSuccessful = false;
				}

				progmemAddr += PAGE_SZ_BYTES;
				bytesLeftToWrite -= thisPageSz;
			}

			if (programWriteSuccessful)
			{
				// Write special page before the bootloader
				boot_page_erase_safe(progmemAddr);

				progmemAddr = BOOTLOADER_ADDRESS - PAGE_SZ_BYTES;
				for(uint8_t i=0; i<PAGE_SZ_BYTES; i+=2)
				{
					uint16_t w = 0xFFFF;
					if (i == PAGE_SZ_BYTES - 2)
						w = appResetRJMP;
					boot_page_fill_safe(progmemAddr + i, w);
				}
				boot_page_write_safe(progmemAddr);
				boot_spm_busy_wait();

				// Verify
				if (appResetRJMP != pgm_read_word(BOOTLOADER_ADDRESS - 2))
					programWriteSuccessful = false;
			}

			// Did we succeed?  Write eeprom crc
			if (programWriteSuccessful)
			{
				eeprom_write_dword((uint32_t*)EEPROM_PROG_CRC32_BASE, crc32_spiflash);
				eeprom_busy_wait();
			}
		}
	} else if (0xFFFFFFFF == eeprom_read_dword((const uint32_t*)EEPROM_PROG_CRC32_BASE)) {
		// If there's no ISPL image and there doesn't appear to be anything bootloaded, just beep
		beep();
	}

	// this will jump to user app
	asm volatile("rjmp (__vectors - 2)");
}

