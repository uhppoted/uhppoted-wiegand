# TODO

- [x] Update README

## _wiegand-emulator_
   - [x] Seperate build
   - [x] MODE: EMULATOR
   - [x] Remove door unlock stuff
   - [x] Remove ACL
   - [x] Remove SD card support
   - [x] Update CLI
   - [x] Commonalise cardf(...)
   - [x] Fritzing
   - [x] Commonalise CLI
   - [x] Commonalise setup stuff
   - [ ] USB

## _wiegand-controller_
   - [x] Seperate build
   - [x] MODE: CONTROLLER
   - [x] Remove emulator stuff
   - [x] Update CLI
   - [x] Reimplement controller card access
   - [x] Fritzing
   - [x] Commonalise setup stuff
   - [ ] USB

## Base
   - [ ] USB
   - [ ] Buzzer
   - [ ] PicoW+TCP/IP
         - https://www.scaprile.com/2023/02/05/on-hardware-state-machines-how-to-write-a-simple-mac-controller-using-the-rp2040-pios
   - [ ] Pico + DTMF

   - (?) Authorisation
   - [ ] tweetnacl
         - [ ] TCP/IP
         - (?) ACL
         - (?) Auth

## Schematic
- RD/WR jumpers
- Mode jumpers
- Buzzer
  - through-hole so that it can be routed to enclosue
- LEDs 
  - through-hole so that they can be routed to enclosue
- Reset button
- SD card
- (?) Long line drivers
- (?) PiZero header

## Other
1. [NuttX](https://nuttx.apache.org/docs/latest/platforms/index.html)


## NOTES

1. Delta time for card read is 53ms (~2ms/bit)
2. PIO clock 
   - 125MHz   -> 8ns per instruction
   - nop [32] -> 256ns  => 0.256us
   - 32x loop -> 8192ns => 8.192us
   - div 64 => 0.5ms
   - configured at 152.588 (~1.25s)
   - maximum div for reliable reads is 192 (~1.6ms)

3. https://github.com/teslamotors/liblithium
4. https://www.mouser.ca/ProductDetail/Microchip-Technology-Atmel/ATECC608A-MAHDA-T?qs=wd5RIQLrsJiNcP5AuKx6KQ%3D%3D
   - https://www.sparkfun.com/products/18077
   - https://www.mikroe.com/secure-4-click
5. Lua
   - https://github.com/kevinboone/luapico
   - https://sillycross.github.io/2022/11/22/2022-11-22/

6. TPIC6B595
   - https://www.ti.com/lit/an/slpa004a/slpa004a.pdf
   - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
   - https://www.appelsiini.net/2012/driving-595-shift-registers/#spcr
   - https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1133532/tpic6595-relay-back-emf

7. Level shifters
   - https://www.tindie.com/products/ddebeer/12v-level-shifter-and-buffer-2x6-channel
   - https://electronics.stackexchange.com/questions/486571/12v-to-3-3-v-logic-level-shifter

8. Relay drivers
   - https://electronics.stackexchange.com/questions/585207/uln2003-relay-driver
   - https://electronics.stackexchange.com/questions/258081/uln2003-used-for-security-door-magnets
   - https://electronics.stackexchange.com/questions/357351/shift-register-that-accepts-3-3v-to-drive-several-5v-relays
   - https://electronics.stackexchange.com/questions/505318/how-to-properly-use-a-relay-module-with-jd-vcc-from-arduino-raspberry/508672#508672
   - https://raspberrypi.stackexchange.com/questions/118604/using-single-5v-relay-module-jqc-3ff-s-z-asking-for-help
