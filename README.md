# ISPL Bootloader / Compiler / Flasher Tools

This project contains shared tools for programming SoundBytes Plus-style
projects.  Right now that's the SoundBytes Plus itself as well as the
ckt-xing-basic.


# Getting Things Set Up

So there's a few things you're going to need in order to build and program SoundBytes Plus modules


## Python 3.11

I highly recommend running this in a venv, and you're really probably going to want python 3.11 or better for performance reasons.  Also that's what I've tested with.  It probably will run under other versions, but you're kind of on your own there.  This is all going to assume you're running Ubuntu or some other Debian-based distro.

```
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install python3.11 python3.11-venv
```

Now, set up that venv and activate it.  I build mine in the sbplus/tools directory, so it's ckt-dingdong/src/sbplus/tools/sbp-venv...

```
python3.11 -m venv sbp-venv
source sbp-venv/bin/activate
```

## Additional Python Packages

Now that you've activated your venv (right?!?), we need to install some dependencies.  They're in sbplus/tools/requirements.txt
```
python -m pip install -r sbplus/tools/requirements.txt
```

# Building and Flashing

## Bootloader

The SoundBytes Plus bootloader was intended so that we'd never have to flash both the AVR and the SPI sound chip again.  The firmware gets embedded into the final .bin image loaded to the flash chip, and then it gets checked / pulled over at run time.

But you have to flash the bootloader to it first.  It's in sbplus-bootloader.  Set the programmer switch to AVR and run the usual command to load it:
```
make fuse flash
```

## SoundBytes Plus Projects

Under the sbplus directory are directories with sound projects.  These consist of two pieces - the source code for the AVR in (project)/src and audio assets in audio/.  It's tied together with the program.ispl file.  There's a master makefile in the top level project directory.  Just running make will build both firmware and the project ISPL binary (usually project.sbp or some such).  If you switch the programmer over to SPI flash mode and run make flash, it'll program the Winbond SPI flash on the board.

 
