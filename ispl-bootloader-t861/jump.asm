;*************************************************************************
;Title:    SoundBytes Plus Bootloader Vector Table
;Authors:  Michael Petersen <railfan@drgw.net>
;          Nathan Holmes <maverick@drgw.net>
;File:     jump.asm
;License:  GNU General Public License v3
;
;LICENSE:
;    Copyright (C) 2023 Nathan Holmes and Michael Petersen
;
;    Many of the concepts in here were borrowed from the GemmaBoot project by Frank Zhao and Adafruit
;
;    This program is free software; you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation; either version 3 of the License, or
;    any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;    
;    You should have received a copy of the GNU General Public License along 
;    with this program. If not, see http://www.gnu.org/licenses/
;    
;*************************************************************************

.org 0x0000
		rjmp BOOTLOADER_ADDRESS
		; The bootloader doesn't use any other vectors, so no need to remap them

main:	cli
		rjmp .-2
