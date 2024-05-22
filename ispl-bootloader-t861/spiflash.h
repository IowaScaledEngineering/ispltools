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
#ifndef _SPIFLASH_H_
#define _SPIFLASH_H_

#include <stdint.h>
#define SPI_DDR_PORT DDRA
#define SPI_PORT     PORTA
#define USI_DI_PIN   PA0
#define USI_DO_PIN   PA1
#define USI_SCK_PIN  PA2
#define USI_CS_PIN   PA3

#define SPI_FLASH_READ_CMD       0x03
#define SPI_FLASH_POWER_UP_CMD   0xAB
#define SPI_FLASH_READ_UUID      0x4B
#define UUID_LEN_BYTES           8

#define min(a,b) ((a)<(b)?(a):(b))
void spiSetup();
void spiCSEnable();
void spiCSDisble();
uint8_t spiTransferByte(uint8_t txData);
void spiflashReset();
void spiflashReadBlock(uint32_t addr, uint8_t len, uint8_t* destPtr);
void spiflashReadUUID(uint8_t* destPtr, uint8_t len);
uint8_t spiflashReadU8(uint32_t addr);
uint16_t spiflashReadU16(uint32_t addr);
uint32_t spiflashReadU32(uint32_t addr);
#endif
