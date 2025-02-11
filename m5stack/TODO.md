# TODO

## Schematic
- [x] WaveShare RP2040-Tiny

- [x] WS2812B-2020
      - [x] SN74HCT1G125
      - [x] Decoupling capacitors
      - [x] Reservoir capacitor
      - [x] Input resistor
      - [X] Pullups
      - https://blog.adafruit.com/2023/07/06/smd5050-led-mount-light-pipe-3dthursday-3dprinting
      - https://electronics.stackexchange.com/questions/408310/attaching-a-3-mm-fiber-optic-cable-to-a-ws2812-led

- [ ] Isolated I/O
      - [x] Decoupling capacitors
      - [x] Inputs
      - (?) Protection diodes
      - https://www.ti.com/product/ISOW7842
      - https://www.mouser.ca/ProductDetail/Texas-Instruments/ISOW7842FDWER

- [ ] Line Driver 
      - [ ] Change to ISOW7841 and use additional output for OE
      - [x] LTSpice model
      - [x] SN74LVC2G126-EP
            - [x] Move OE to other side of symbol
      - [x] Current limiting resistors 
      - [x] Decoupling capacitors
      - [x] MOVs
      - (?) Protection diodes
      - https://www.ti.com/product/SN74LVC2G126-EP

- [ ] 12V to 5V conversion

- [ ] Power:
      - [ ] M5 bus + external
            - https://www.ti.com/product/TPS51388
            - https://www.ti.com/product/TPS629203
      - [ ] M5 bus only
      - [ ] USB
      - [ ] Isolated grounds
- (?) USB mounting holes

## PCB

## Notes

1. https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/M5-Core-Schematic(20171206).pdf
2. https://www.adafruit.com/product/4978