# M5Stack Wiegand Emulator

RP2040 _M5Stack_ module to emulate a Wiegand reader:

- Emulates a single Wiegand-26 card reader/keypad with an LED. 
- Implements the M5Stack serial interface.
- Intended for use with an M5 _base_ module but can optionally be powered via the external connector
  for standalone use.

## Raison d'être

Mostly to avoid having to dig out a breadboard and figure how it worked all over again every time I needed
to test a Wiegand interface.

But also useful for:
- automated testing
- testing and implementation of protocol variants

## Notes

1. The emulator uses a line driver to interface to the external controller i.e. it is not an open-collector
   and cannot be "parallelised" (as in e.g. [Will this circuit for two Wiegand card readers allow them to talk to one control board?](https://electronics.stackexchange.com/questions/535159/will-this-circuit-for-two-wiegand-card-readers-allow-them-to-talk-to-one-control). Which TBH is kindof a weird thing to do but ..).

## Status

**IN DEVELOPMENT**

## Design Notes

### Rev.0

### Notes
1. The RP2040 is **way** overkill for this application but makes for a nice development setup.
2. It's more logicial to have the ISO784x on the other side of the RP2040 but for now it allows for powering via
   USB during development.
3. Measured current draw: 55mA

#### Errata
1. Pads for SK6812 are incorrectly numbered.
2. Pads for D2, D3 and D4 are incorrect (anode and cathode swapped).
3. ISO7841 doesn't allow RC filter on LED input.
4. Insufficient clearance around mounting holes.
5. DI and DO are pulled low on power-up (should default to _high_).
6. Solder mask does not include M5 connectors.
7. GPIO pins are incorrect (IO1 and IO2 instead of IO6 and IO7).

## References

1. [M5Stack](https://m5stack.com)
2. [WaveShare RP2040-Tiny](https://www.waveshare.com/wiki/RP2040-Tiny)
3. [M5 Core Schematic(20171206)](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/M5-Core-Schematic(20171206).pdf)
