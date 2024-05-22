/*************************************************************************
Title:    Small Sound Player Module - SPI Flash Interface
Authors:  Michael Petersen <railfan@drgw.net>
          Nathan D. Holmes <maverick@drgw.net>
          Based on the work of David Johnson-Davies - www.technoblogy.com - 23rd October 2017
           and used under his Creative Commons Attribution 4.0 International license
File:     $Id: $
License:  GNU General Public License v3

CREDIT:
    The basic idea behind this playback design came from David Johson-Davies, who
    provided the basic framework and the place where I started.

LICENSE:
    Copyright (C) 2021 Michael Petersen, Nathan Holmes

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
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include "spiflash.h"

void spiSetup()
{
	USIPP = _BV(USIPOS); // Put USI pins on PORTA
	
	USICR = _BV(USIWM0) | _BV(USICS1) | _BV(USICLK);

	// SCK and DO should be outputs, DI input
	SPI_DDR_PORT |= _BV(USI_SCK_PIN);
	SPI_DDR_PORT |= _BV(USI_DO_PIN);
	SPI_DDR_PORT &= ~_BV(USI_DI_PIN);

	SPI_PORT     |= _BV(USI_CS_PIN);  // Set /CS high, just in case
	SPI_DDR_PORT |= _BV(USI_CS_PIN);  // Make it an output
}

inline void spiCSEnable()
{
	SPI_PORT &= ~(_BV(USI_CS_PIN));  // Set /CS low (enable)
}

inline void spiCSDisble()
{
	SPI_PORT |= _BV(USI_CS_PIN);  // Set pin high (disable)
}

uint8_t spiTransferByte(uint8_t txData)
{
	USIDR = txData;
	USISR = 0;

	// Faster than a loop is just to run this 16x to create all the clock edges
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	USICR |= _BV(USITC);
	
	return USIDR;
}

void spiflashReset()
{
	uint8_t i;
	_delay_us(5);
	spiCSEnable();
	spiTransferByte(SPI_FLASH_POWER_UP_CMD);
	for(i=0; i<3; i++)
	{
		spiTransferByte(0xFF);
	}
	spiCSDisble();
	_delay_us(5);
}

void spiflashReadUUID(uint8_t* destPtr, uint8_t len)
{
	uint8_t i;
	spiCSEnable();
	spiTransferByte(SPI_FLASH_READ_UUID);

	// Dummy bytes for read cycle
	for(i=0; i<4; i++)
		spiTransferByte(0xFF);

	len = min(len, 8);

	for(i=0; i<len; i++)
	{
		*destPtr++ = spiTransferByte(0xFF);
	}
	spiCSDisble();
}


void spiflashReadBlock(uint32_t addr, uint8_t len, uint8_t* destPtr)
{
	uint8_t i;
	spiCSEnable();
	spiTransferByte(SPI_FLASH_READ_CMD);
	spiTransferByte(0xFF & (addr>>16));
	spiTransferByte(0xFF & (addr>>8));
	spiTransferByte(0xFF & addr);
	for(i=0; i<len; i++)
	{
		*destPtr++ = spiTransferByte(0xFF);
	}
	spiCSDisble();
}

uint8_t spiflashReadU8(uint32_t addr)
{
	uint8_t i;
	spiCSEnable();
	spiTransferByte(SPI_FLASH_READ_CMD);
	spiTransferByte(0xFF & (addr>>16));
	spiTransferByte(0xFF & (addr>>8));
	spiTransferByte(0xFF & addr);
	i = spiTransferByte(0xFF);
	spiCSDisble();
	return i;
}

uint16_t spiflashReadU16(uint32_t addr)
{
	uint16_t i;
	spiCSEnable();
	spiTransferByte(SPI_FLASH_READ_CMD);
	spiTransferByte(0xFF & (addr>>16));
	spiTransferByte(0xFF & (addr>>8));
	spiTransferByte(0xFF & addr);
	i = ((uint16_t)spiTransferByte(0xFF))<<8;
	i |= spiTransferByte(0xFF);

	spiCSDisble();
	return i;
}

uint32_t spiflashReadU32(uint32_t addr)
{
	uint32_t i=0, j;
	spiCSEnable();
	spiTransferByte(SPI_FLASH_READ_CMD);
	spiTransferByte(0xFF & (addr>>16));
	spiTransferByte(0xFF & (addr>>8));
	spiTransferByte(0xFF & addr);
	for(j=0; j<4; j++)
	{
		i <<= 8;
		i |= spiTransferByte(0xFF);
	}
	spiCSDisble();
	return i;
}



