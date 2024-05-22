#!/usr/bin/python3

from pyftdi.ftdi import Ftdi

Ftdi.show_devices()

from pyftdi.ftdi import Ftdi
from pyftdi.misc import hexdump, pretty_size
from pyftdi.spi import SpiController
from pyftdi.usbtools import UsbTools
from serialflash import SerialFlashManager
import mmap
import sys

def test():
    url = 'ftdi://ftdi:232h:1/1'
    speed = 12E6
    flash = SerialFlashManager.get_flash_device(url, 1, speed)
    print("Flash device: %s @ SPI freq %0.1f MHz ID:[]" % (flash, speed / 1E6))

def write(fname):
    url = 'ftdi://ftdi:232h:1/1'
    speed = 12E6
    flash = SerialFlashManager.get_flash_device(url, 1, speed)
    print("Flash device: %s @ SPI freq %0.1f MHz ID:[]" % (flash, speed / 1E6))

    with open(fname, 'rb') as fh:
        m = mmap.mmap(fh.fileno(), 0, access=mmap.ACCESS_READ)
        ba = bytearray(m)
        print("Unlocking...")
        flash.unlock()
        flashLen = len(ba)

        print("Erasing %d bytes..." % (flashLen))
        eraseLen = 4096
        while eraseLen < flashLen:
            eraseLen += 4096

        flash.erase(0x0, eraseLen)


        print("Writing %d bytes..." % (flashLen))
        flash.write(0x0, ba)

        print("Reading...")
        readback = flash.read(0x0, flashLen)
        
        print("Verifying...")
        success = 0
        fail = 0
        for i in range(0, flashLen):
            if readback[i] != ba[i]:
                print("Byte %d does not verify" % i)
                fail += 1
            else:
                success += 1
        if fail == 0:
            print("SUCCESS!  %d bytes written successfully" % success)
        else:
            print("FAIL!  %d bytes good, %d bytes BAD" % (success, fail))

if __name__ == '__main__':
    write(sys.argv[1])
