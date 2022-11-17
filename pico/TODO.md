# TODO

1. [x] Replace `tight_loop_contents` with PIO FIFO interrupt
2. [x] JMP loop
3. [ ] Debounce 
3. [ ] Reset bit decoder on timeout
4. [ ] Join FIFOs
5. [ ] gpio_set_function(0, GPIO_FUNC_PIO0);
6. [ ] Move LED from PIO to main loop
       - green for ok
       - red for bad
       - route ok/bad to HID
7. [ ] Replace sleep with queue wait/interrupt
       - (?) blink in timer interrupt or something
8. [ ] Alternative implementations
       - Use two SMs and WAIT pin
       - Accumulate 26 bits in ISR
9. [ ] Query last read + delta

## NOTES

1. https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer