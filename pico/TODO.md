# TODO

1.  [x] Reset bit decoder on timeout
2.  [x] Join FIFOs
3.  [x] Debounce properly
4.  [x] Signal read timeout
5.  [x] Status LEDs
6.  [x] Replace sleep with queue wait/interrupt
7.  [x] Move reader to its own file
8.  [x] Restructure folders to use src and include
9.  [x] https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
10. [x] ~~Alternative implementations using two SMs and WAIT pin~~
11. [x] Move remaining card stuff to reader.c
12. [x] README
13. [x] Mode reader/emulator/unknown
14. [x] CLI
15. [ ] Emulator
        - [x] CLI WRITE command
        - [x] writer_write
        - [x] WRITE.pio
        - [x] PIO/shift-and-set
        - [x] Card readback
        - [x] Set bit delay to 1ms (x2)
        - [x] Not formatting card number correctly to actual controller
```
405419896  ... 2022-12-20 12:27:44 0          0 00 00 | 206924 1   false 3 1     10058399   2022-12-20 12:27:44 6
405419896  ... 2022-12-20 12:29:16 0          0 00 00 | 206925 1   false 4 1     1058399    2022-12-20 12:29:16 6

reader:
>> DEBUG: 13223999 00c9c83f

emulator:
>> DEBUG: 34981951 0215c83f
```

        - [ ] Ignore if not emulator mode
        - [ ] QUERY - show R/W
        - [ ] startup glitch

16. [ ] Prototype schematic
        - LEDs
        - Move all the Wiegand I/O to one side
        - Relay driver
        - Door input
        - Door relay
        - (?) Fritzing

17. [ ] Schematic
        - Relay driver
        - Door input
        - Door relay
        - LEDs
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