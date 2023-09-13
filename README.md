![build](https://github.com/uhppoted/uhppoted-wiegand/workflows/build/badge.svg)

# uhppoted-wiegand

`uhppoted-wiegand` implements a _Raspberry Pi Pico_ reader/emulator for a Wiegand-26 interface.

The project is **mostly** a just-for-fun exploration of the capabilities of the RP2040 PIO but could also 
be useful as a basis for:

- testing and debugging
- interfacing non-standard/off-brand readers and keypads to the UHPPOTE L0x access controllers

The project includes:
- PIO based Wiegand-26 reference reader/writer implementation
- PIO based Wiegand-26 reader emulator which can write a Wiegand-26 card out to a controller
- PIO based Wiegand-26 controller emulator which can read from Wiegand-26 reader and unlock a door

## Raison d'être

The RP2040 PIO is an intriguing peripheral and this project was an excuse to explore its capabilities and
limitations. Wiegand-26 is particularly simple protocol and does not even begin to push the boundaries of
the PIO but the code and associated information may be useful for other things.

## Status

Breadboarded and Fritzing'ed implementations of:
- Wiegand-26 reference reader/writer
- Wiegand-26 reader emulator
- Wiegand-26 controller emulator

Next up:
- Wiegand-26 relay
- (maybe) KiCad schematics

<img width="800" src="documentation/images/fritzing-reference.png"> 

## Releases

| *Version* | *Description*                                                                             |
| --------- | ----------------------------------------------------------------------------------------- |
| v0.8.6    | Reworked to use on-board flash for ACL (with SD card override)                            |
| v0.8.5    | Added support for PicoW and a telnet CLI                                                  |
| v0.8.4    | Initial release of reference, reader emulator and controller emulator                     |


### Building from source

Required tools:
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- Compatible C/C++ compiler
- _make_
- [PicoProbe](https://github.com/raspberrypi/picoprobe) (optional but recommended)
- [OpenOCD](https://github.com/raspberrypi/openocd.git) (optional but recommended)
- [PicoTool](https://github.com/raspberrypi/picotool) (optional but recommended)

To build using the included _Makefile_:
```
git clone https://github.com/uhppoted/uhppoted-wiegand.git
cd uhppoted-wiegand/pico
make clean
make build
```

The `build` target produces both a UF2 and an ELF binary for installation on a _Raspberry Pi Pico_ using either:
- USB (using _BOOTSEL_)
- _PicoProbe_ + _OpenOCD_
- _PicoTool_

- The `make install` command installs the binary using _PicoTool_
- The `make run` command installs the binary using _PicoProbe_ and _OpenOCD_

#### Dependencies

| *Dependency*                                                    | *Description*                       |
| --------------------------------------------------------------- | ----------------------------------- |
|                                                                 |                                     |
|                                                                 |                                     |

## Usage

The provided software includes a **very** bare bones serial port command interface. Commands are (currently)
single letter mnemonics and need to be terminated by a carriage return and/or line feed.

The supported command set comprises:

| *Command*      | *Description*                           |
| -------------- | --------------------------------------- |
| T              | Set date/time (YYYY-MM-DD HH:mm:ss)     |
| Wnnnnnnnn      | Emulates card swipe                     |
| Kdddddddd      | Emulates keypad                         |
|                |                                         |
| GRANT nnnnnn   | Grant card access rights                |
| REVOKE nnnnnn  | Revoke card access rights               |
| LIST ACL       | List cards in ACL                       |
| READ ACL       | Read ACL from SD card                   |
| WRITE ACL      | Write ACL to SD card                    |
| QUERY          | Display last card read/write            |
|                |                                         |
| MOUNT          | Mount SD card                           |
| UNMOUNT        | Unmount SD card                         |
| FORMAT         | Format SD card                          |
|                |                                         |
| UNLOCK         | Unlocks door for 5 seconds              |
| OPEN           | Emulates door open contact              |
| CLOSE          | Emulates door close contact             |
| PRESS          | Emulates pushbutton press               |
| RELEASE        | Emulates pushbutton release             |
|                |                                         |
| BLINK          | Blinks reader LED 5 times               |
| CLS            | Reinitialises terminal                  |
| REBOOT         | Reboot                                  |
| ?              | Display list of commands                |


Notes:| |
1. The default facility code for _emulator_ mode is a build time constant (`FACILITY_CODE` in the _Makefile_) and will
be used if the _W_ command card number is 5 digits or less.


## References and Related Projects

1. [Getting started with the Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
