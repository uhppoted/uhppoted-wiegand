# TODO

## Schematic
    - [ ] ERC

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
    - [ ] Routing
          - [x] ~~Gah! Swap front and back sides :-( :-( :-(~~
          - [x] J3 is flipped
          - [x] D2/D3/D4 should all be oriented the same way
          - (?) rotate U1/U2
          - [ ] TranzOrbs 
                - (?) check spacing
                - (?) 3D model
                - (?) revert to THT
                - respec for transient protection + e.g. 5mA pin current
          - (?) THT resistors for 10/24/32mA options

## Notes

1. https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/M5-Core-Schematic(20171206).pdf
2. https://www.mouser.ca/ProductDetail/Texas-Instruments/ISOW7841FDWER
3. https://www.ti.com/product/SN74LVC2G126-EP
4. https://www.adafruit.com/product/4978
5. https://blog.adafruit.com/2023/07/06/smd5050-led-mount-light-pipe-3dthursday-3dprinting
6. https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led
7. https://forum.digikey.com/t/specify-a-simple-drill-hole-in-pcbnew/4825
