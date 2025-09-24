# NOTES (Rev. 1)

- https://www.vinthewrench.com/p/raspberry-pi-internet-of-things-part-2a8

1.  Use 2mmm or 2.54mm jumpers (and push through when soldering so that jumper is flush)
2.  Maybe use JST-SH connector (through hole?)
     - 1.0mm pitch JST SH connectors
     - 1.25mm pitch Molex MicroBlade
     - https://www.pcboard.ca/jst-sh-5-pin-cable
     - (?) use GROVE (standard for M5)
3.  SK6812 pad numbering incorrect
4.  Remove C1 - ISOW7841 requires fast inputs
   - either leave off altogether
   - add Schmitt trigger
5. IO6 and IO7 are assigned to GPIO2 and GPIO1
6.  Increase R8 - Vdd*R8/(R2+R13+R8) is only 4.1V
7.  ISOW784xF series has 1.5M pulldowns on inputs (pullups without F-suffix).
8.  ISOW784x datasheet recommends 0.1uF in parallel with 22uF decoupling on power pins.
   - also: see layout guidelines
   - also: https://e2e.ti.com/support/isolation-group/isolation/f/isolation-forum/836457/isow7841-isow7841-breaks-down/3107471#3107471
   - also: https://e2e.ti.com/support/isolation-group/isolation/f/isolation-forum/846942/isow7841-isow7841-breaks-down

9.  Replace resistor network with discrete resistors (many unused/easier to solder)
10.  [ADuM320N](https://www.mouser.ca/datasheet/2/609/adum320n_321n-3420518.pdf) instead of ISOW7843
11. LED light pipes
     - https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led
     - https://www.mouser.ca/c/optoelectronics/led-indication/led-light-pipes/?number%20of%20elements=1%20Element&orientation=Right%20Angle&pg=2
     - (?) right angle LED
     - https://www.aliexpress.com/item/1005004538582988.html
     - https://blog.adafruit.com/2023/07/06/smd5050-led-mount-light-pipe-3dthursday-3dprinting
     - https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led

12. Relocate R9 closer to U3 and swap pins.
13. Make JP4-7 normally closed so they just have to be cut if necessary (i.e. no soldering iron required).
14. Larger holes for test points.
15. Longer jumpers (or push pins through header before soldering)
16. MSP4.0A footprint is incorrect (pins are swapped)
17. Move resistor network so that RP2040 can be optionally mounted directly on board (castellated)
18. Replace WS2812B with APA102-S/SK9822/NS107S (SPI) - and 2020 package (5050 is **huge**)
    - [SK6812-SIDE](https://www.adafruit.com/product/4691)
    - txb0108
    - SN74LV1T125
    - 74LVC3G34
    - 74AHCT1G125
    - SN74LVC1T45
    - https://www.eevblog.com/forum/beginners/translate-3v-spi-to-5v-spi
    - https://protosupplies.com/product/hi-speed-spi-logic-level-converter-module
    - https://electronics.stackexchange.com/questions/18186/cheapest-way-to-translate-5v-spi-signal-to-3v-spi
    - https://www.adafruit.com/product/757
    - https://electronics.stackexchange.com/questions/378574/common-cathode-rgb-led-from-3-3v
    - https://www.lumissil.com/assets/pdf/core/IS31FL3194_DS.pdf
    - https://os.mbed.com/users/4180_1/notebook/rgb-leds
    - https://electronics.stackexchange.com/questions/48522/what-resistor-to-use-with-this-rgb-led
    - https://electronics.stackexchange.com/questions/373148/resistor-requirement-for-3-3-v-supply
19. Fix weirdly crossed signal lines between ISOW and line buffer
20. Relook at ESD/protection diodes
    - https://hackaday.com/2025/06/19/hacker-tactic-esd-diodes

21. Rotate connector side to match protoboard convention.
22. Additional clearance around mounting holes
23. Tie DI and DO high.
24. Include M5 connectors in solder mask.
25. Make **every** component hand solderable (looking at you D1 and U6 :-()
26. Buzzer IO
27. SMT headers
   - https://electronics.stackexchange.com/questions/415542/are-these-tiny-vertical-dual-row-pcb-mounted-connector-pins-standard/719876#719876
28. PCB notch for CoreS3 power switch
29. Slot in enclosure for USB cable

- https://hackaday.com/2025/09/09/freecad-foray-from-brick-to-shell/
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
