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
11. [ ] SPI
        - [x] last card + timestamp
        - [ ] set time

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