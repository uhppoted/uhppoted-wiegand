# TODO

## Reader/Writer
- [x] 'beep' on write
- [ ] Rename READER/EMULATOR
      - (?) CONTROLLER/READER maybe
- [ ] Watchdog reset
- [ ] Buzzer
- [ ] Relay
- [ ] Button
- [ ] SD card

## Breadboard schematic
- [ ] Fritzing
- LEDs
- Relay driver
- Door input
- Door relay
- Buzzer
- (?) SD card
- (?) Photo

## Schematic
- RD/WR jumpers
- Mode jumpers
- Relay driver
- Door input
- Door relay
- Buzzer
- LEDs
- Reset button
- (?) Long line drivers
- (?) PiZero header
- (?) SD card

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
