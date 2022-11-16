# TODO

1. [x] Replace `tight_loop_contents` with PIO FIFO interrupt
2. [x] JMP loop
3. [ ] Reset SM on timeout
4. [ ] gpio_set_function(0, GPIO_FUNC_PIO0);
5. [ ] Move LED from PIO to main loop
       - green for ok
       - red for bad
       - route ok/bad to HID
6. [ ] Join FIFOs
7. [ ] Replace sleep with queue wait/interrupt
       - (?) blink in timer interrupt or something
8. [ ] Alternative implementation using two SMs and WAIT pin

## TODO
1. (?) Wired-OR for D0+D1 