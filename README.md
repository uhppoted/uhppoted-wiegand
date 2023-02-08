# uhppoted-wiegand

`uhppoted-wiegand` implements a _Raspberry Pi Pico_ reader/emulator for a Wiegand-26 interface.

The project is **mostly** a just-for-fun exploration of the capabilities of the RP2040 PIO but could also 
be useful as a basis for:

- testing and debugging
- interfacing non-standard/off-brand readers and keypads to the UHPPOTE L0x access controllers

The project includes:
- PIO based Wiegand-26 reader
- PIO based Wiegand-26 emulator
- _basic_ serial port command interface
- _(in progress) KiCad schematic for breadboard prototypes_

## Raison d'être

The RP2040 PIO is an intriguing peripheral and this project was an excuse to explore its capabilities and
limitations. Wiegand-26 is particularly simple protocol and does not even begin to push the boundaries of
the PIO but maybe the code and associated information will be useful for other things.

## Status

_\<sigh\> ... Fritzing's Lament_: 

_How many times must a man shuffle his  
Breadboard layout again?  
And how many wires must be trim'd and cut  
Before no connection is unsound?  
Oh, how many times will the LEDs be  
Completely the wrong way around?  
The answer, my friend  
Is blowing in the wind...  
The answer is blowin' in the wind!_

_Oh, how many years can a bug lurk and hide  
Before it comes to the fore?  
And how many years can some people exist  
Happily ignorant of circuitry  
Oh, how many times must a man type ===  
Before val is really truthy?  
The answer, my friend  
Is blowing in the wind...  
The answer is blowin' in the wind!_

_And how many times can a man hook up  
VSS straight to the GND?  
And recalculate the series resistance  
So that the relays don't fry?  
Oh, how many times must a man palm his face  
And ask "Oh god, f&@*&%@ why???"  
The answer, my friend, is blowin' in the wind,  
The answer is blowin' in the wind._

## Releases

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

| *Command*              | *Description*                           |
| ---------------------- | --------------------------------------- |
| O                      | Turns on the reader LED                 |
| X                      | Turns off the reader LED                |
| Q                      | Retrieves the last read card (if any)   |
| Tyyyy-mm-dd HH:mm:ss   | Sets the Pico date and time             |
| Wnnnnnnnnn             | Writes a card number out as Wiegand-26  |
| Gnnnnnnnnn             | Grants a card access                    |
| Rnnnnnnnnn             | Revokes a card access                   |
| L                      | Lists cards in the access control list  |

Notes:
1. The default facility code for _emulator_ mode is a build time constant (`FACILITY_CODE` in the _Makefile_) and will
be used if the _W_ command card number is 5 digits or less.


## References and Related Projects

1. [Getting started with the Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
