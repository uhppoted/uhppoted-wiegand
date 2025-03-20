# TODO

## Schematic
    - [x] ERC

## PCB
    - [x] Replace SMD TranzOrbs with through-hole (for hand soldering)
    - [x] DC converter module
    - [x] 10/24/32mA options
    - [x] Lose the TX/RX jumpers and connect both UARTs
    - [x] Solder jumpers on back of board for TX/RX
    - [x] Check M5 stack TX/RX voltage levels
    - [x] Use ISO7843 to drive WS2812
    - [x] Revert to SMD TranzOrbs
    - [x] Reinstate D4 TranzOrb
    - [x] Fix courtyard for RP2040 (should only surround actual pins)
    - [x] Change footprint for off-board connector
    - [x] 3D model for Molex connector
    - [x] Logo + revision
    - [x] Fiducial marks
    - [x] Routing
    - [x] DRC
    - [x] Redo M5 bus connector for hand-soldering

## Design Review
   - [x] Merge areas
   - [x] ~~Vias on the TX/RX jumpers~~
   - [x] Fix J1/J2 pads to include through holes
   - [x] Add mounting holes to J1/J2
   - [x] Fix track to U1/16
   - [x] Cleanup tracks and vias
   - [x] Check WS2812B footprint + pinout
   - [x] Check U2 footprint
   - (?) ~~Rework layout with U2 at top, J1/LED switched~~

## Rev.1
- (?) https://www.ti.com/product/MSPM0C1104
- (?) Add HWPR as optional power

## Notes

1. https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/M5-Core-Schematic(20171206).pdf
2. https://www.mouser.ca/ProductDetail/Texas-Instruments/ISOW7841FDWER
3. https://www.ti.com/product/SN74LVC2G126-EP
4. https://www.adafruit.com/product/4978
5. https://blog.adafruit.com/2023/07/06/smd5050-led-mount-light-pipe-3dthursday-3dprinting
6. https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led
7. https://forum.digikey.com/t/specify-a-simple-drill-hole-in-pcbnew/4825
