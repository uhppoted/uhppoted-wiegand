# M5Stack Wiegand Emulator

RP2040 _M5Stack_ module to emulate a Wiegand reader:

- Emulates a single Wiegand-26 card reader/keypad with an LED. 
- Implements the M5Stack serial interface.
- Intended for use with an M5 _base_ module but can optionally be powered via the external connector
  for standalone use.

## Raison d'être

Mostly to avoid having to dig out a breadboard and figure how it worked all over again every time I needed
to test a Wiegand interface.

## Notes

1. The emulator uses a line driver to interface to the external controller i.e. it is not an open-collector
   and cannot be "parallelised" (as in e.g. [Will this circuit for two Wiegand card readers allow them to talk to one control board?](https://electronics.stackexchange.com/questions/535159/will-this-circuit-for-two-wiegand-card-readers-allow-them-to-talk-to-one-control). Which TBH is kindof a weird thing to do but ..).

## Status

**IN DEVELOPMENT**

## Design Notes

### Rev.0

1. The RP2040 is **way** overkill for this application but makes for a nice development setup.
2. It's more logicial to have the ISO784x on the other side of the RP2040 but for now it allows for powering via
   USB during development.

## References

1. [M5Stack](https://m5stack.com)
2. [WaveShare RP2040-Tiny](https://www.waveshare.com/wiki/RP2040-Tiny)