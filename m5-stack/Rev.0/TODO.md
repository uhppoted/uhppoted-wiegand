# TODO

## [x] Schematic
## [x] PCB
## [x] Design Review
## CoreS3
   - [x] GPIO
   - [x] UART1
   - [x] UART2
   - [ ] prettify

## [ ] Firmware
   - [x] CLI
   - [x] LED
   - [x] SK6812
   - [x] Wiegand 
   - [x] GPIO
   - [x] UART
      - [x] uart0
      - [x] uart1
      - [x] card
      - [x] keycode
      - [x] card + keycode
      - [x] error/ok
   - [ ] USB
      - [x] bootsel
      - [ ] card
      - [ ] keycode
      - [ ] card + keycode
      - [ ] error/ok


## [ ] Enclosure
   - https://github.com/uetchy/m5stack-module

```
from machine import Pin
from utility import print_error_msg
import time

# Setup GPIO6 as output
led = Pin(2, Pin.OUT)

while True:
    print("asdfasd", led.value())
    led.value(not led.value())  # Toggle pin state
    time.sleep(2)              # Wait 10 seconds
```