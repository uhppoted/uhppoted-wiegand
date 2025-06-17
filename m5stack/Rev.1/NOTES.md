# NOTES (Rev. 1)

1.  Use 2mmm or 2.54mm jumpers
2.  Maybe use JST-SH connector (through hole?)
   - 1.0mm pitch JST SH connectors
   - 1.25mm pitch Molex MicroBlade
   - https://www.pcboard.ca/jst-sh-5-pin-cable

3.  Replace resistor network with discrete resistors (many unused/easier to solder)
4.  [ADuM320N](https://www.mouser.ca/datasheet/2/609/adum320n_321n-3420518.pdf) instead of ISOW7843
5.  LED light pipes
     - https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led
     - https://www.mouser.ca/c/optoelectronics/led-indication/led-light-pipes/?number%20of%20elements=1%20Element&orientation=Right%20Angle&pg=2
     - (?) right angle LED
     - https://www.aliexpress.com/item/1005004538582988.html
     - https://blog.adafruit.com/2023/07/06/smd5050-led-mount-light-pipe-3dthursday-3dprinting
     - https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led

6.  Relocate R9 closer to U3 and swap pins.
7.  Make JP4-7 normally closed so they just have to be cut if necessary (i.e. no soldering iron required).
8.  Larger holes for test points.
9.  Longer jumpers
10. MSP4.0A footprint is incorrect (pins are swapped)
11. Move resistor network so that RP2040 can be optionally mounted directly on board (castellated)
12. Replace WS2812B with APA102-S (SPI, 3.3V)
13. Fix weirdly crossed signal lines between ISOW and line buffer

- (?) https://www.ti.com/product/MSPM0C1104
- (?) Add HWPR as optional power


## Other
- https://www.digikey.com/en/products/detail/qt-brightek-qtb/QBLP617-IW/4814678
- https://www.sparkfun.com/sparkfun-rgb-addressable-cbi-led-5mm-right-angle.html
- https://www.sparkfun.com/sparkfun-cbi-light-pipe-for-smd-leds-5mm-right-angle.html
- https://www.sparkfun.com/led-rgb-addressable-pth-5mm-clear-5-pack.html
- https://www.mouser.ca/c/?marcom=109216310
- https://www.digikey.ca/en/products/filter/led-indication-discrete/through-hole-right-angle/105?s=N4IgjCBcoGwJxVAYygMwIYBsDOBTANCAG4B2UALgE4CuBIA9lANogAsYYcATBALqEAHclBABlKgEsSAcxABfBUA
- https://www.raspberrypi.com/news/raspberry-pi-poe-injector-on-sale-now-at-25/
