# TODO

## Reader/Writer
- [x] 'beep' on write
- [ ] hwconfig.c
      - [ ] LED timer pins
- [ ] Relay
      - [x] Inputs (from controller)
      - [ ] Outputs (to door)
            - https://raspberrypi.stackexchange.com/questions/118604/using-single-5v-relay-module-jqc-3ff-s-z-asking-for-help
            - https://electronics.stackexchange.com/questions/505318/how-to-properly-use-a-relay-module-with-jd-vcc-from-arduino-raspberry/508672#508672
            - https://leeselectronic.com/en/product/31300-1-relay-digital-module-5v.html
            - https://leeselectronic.com/en/product/18405-relay-module-1-relay-spdt-12vdc-high-low-trigger.html
            - https://electronics.stackexchange.com/questions/585207/uln2003-relay-driver
            - https://electronics.stackexchange.com/questions/258081/uln2003-used-for-security-door-magnets
            - https://www.ti.com/product/TPIC6595
            - https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1133532/tpic6595-relay-back-emf
- [ ] Buzzer
- [ ] Button
- [ ] PicoW+TCP/IP
- (?) Authorisation
- [ ] tweetnacl
      - [ ] TCP/IP
      - (?) ACL
      - (?) Auth

## Breadboard 
- [ ] Fritzing
      - [ ] Door relay driver
- [ ] Photo
- [ ] KiCard schematic
- Door input
- Pushbutton input
- Buzzer
- Pushbutton

## Schematic
- RD/WR jumpers
- Mode jumpers
- Relay driver
- Door input
- Door relay
  - https://www.ti.com/product/TPIC6595
- Buzzer
  - through-hole so that it can be routed to enclosue
- LEDs 
  - through-hole so that they can be routed to enclosue
- Reset button
- SD card
- (?) Long line drivers
- (?) PiZero header

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
