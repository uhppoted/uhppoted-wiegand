# TODO

## Emulator
- [x] PIO for reader LED
      - [x] CLI grant/deny
- [x] Statically allocate PIOs and SMs
- [ ] READER PIO losing synch (or something)
- [ ] startup message
      - delay any other messages until after startup
      - maybe only initialise PIOs after delay
- [ ] startup glitch
- [ ] 'beep' on read
- [ ] Move mode pins down to make space for UART1 on GPIO 4/5

## Breadboard schematic
- [ ] Fritzing
- LEDs
- Relay driver
- Door input
- Door relay
- Buzzer
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
