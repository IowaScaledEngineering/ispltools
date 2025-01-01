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
    try:
        flash = SerialFlashManager.get_flash_device(url, 1, speed)
    except:
        return -1
    print("Flash device: %s @ SPI freq %0.1f MHz ID:[]" % (flash, speed / 1E6))
    return 0


if __name__ == '__main__':
    sys.exit(test())
