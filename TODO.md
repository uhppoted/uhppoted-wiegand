# TODO

## Reference
   - [ ] PicoW+TCP/IP
         - [x] Replace blocking connect
         - [x] Reenable watchdog
         - [x] Listen/accept on connected
         - [x] Retry on connect failed
         - [x] Tidy up link status stuff
         - [x] Route commands/responses to/from CLI
         - [x] Move PicoW stuff to base
         - [x] Reinstate full functionality with TCP/IP
         - [x] Close idle connections
         - [x] Detect connection closed
         - [x] Close connection on accept error
         - [x] Remove tcpd_result
         - [x] Multiple client connections
         - [x] Restart server occasionally on server idle 
         - [x] ~~Send queue~~
         - [x] Merge picow branch back into main
         - [x] Logging client/port
               - [x] Logd listener
               - [x] Ignore CLI requests
               - [x] Route log messages
               - [x] Switch all logging to logd
               - [x] Format CLI command responses
         - [x] cyw43_arch_lwip_check
         - [x] Reduce poll interval
         - [x] Configurable ports 
         - [x] Configurable connect timeout
         - [x] Configurable idle timeout
         - [ ] USB+WiFi variants
               - [ ] Figure out SD card detect interrupt conflict
               - [x] controller
               - [ ] emulator
               - [ ] reference
               - [x] Don't log SENT bytes to TCP log (infinite loop)
         - [ ] Check emulator
         - [ ] Check reference


   - [ ] Buzzer
   - (?) CLI auth
         - HOTP
   - [ ] tweetnacl/TLS
         - [ ] TCP/IP
         - (?) ACL
         - (?) Auth

   - [ ] (optionally) Replace external SD card with internal flash
         - https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_flash
   - (?) DTMF

## Schematic
- RD/WR jumpers
- Mode jumpers
- Buzzer
  - through-hole so that it can be routed to enclosue
- LEDs 
  - through-hole so that they can be routed to enclosue
- Reset button
- SD card
- (?) Long line drivers
- (?) PiZero header

## Other
1. [NuttX](https://nuttx.apache.org/docs/latest/platforms/index.html)
2. [RiotOS](https://www.riot-os.org)

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

6. TPIC6B595
   - https://www.ti.com/lit/an/slpa004a/slpa004a.pdf
   - https://docs.arduino.cc/tutorials/communication/guide-to-shift-out
   - https://www.appelsiini.net/2012/driving-595-shift-registers/#spcr
   - https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1133532/tpic6595-relay-back-emf

7. Level shifters
   - https://www.tindie.com/products/ddebeer/12v-level-shifter-and-buffer-2x6-channel
   - https://electronics.stackexchange.com/questions/486571/12v-to-3-3-v-logic-level-shifter

8. Relay drivers
   - https://electronics.stackexchange.com/questions/585207/uln2003-relay-driver
   - https://electronics.stackexchange.com/questions/258081/uln2003-used-for-security-door-magnets
   - https://electronics.stackexchange.com/questions/357351/shift-register-that-accepts-3-3v-to-drive-several-5v-relays
   - https://electronics.stackexchange.com/questions/505318/how-to-properly-use-a-relay-module-with-jd-vcc-from-arduino-raspberry/508672#508672
   - https://raspberrypi.stackexchange.com/questions/118604/using-single-5v-relay-module-jqc-3ff-s-z-asking-for-help

9. USB
   - https://forums.raspberrypi.com/viewtopic.php?t=331207
   - https://github.com/raspberrypi/pico-examples/issues/19
   - https://forums.raspberrypi.com/viewtopic.php?t=333027
   - https://blog.smittytone.net/2021/10/31/how-to-send-data-to-a-raspberry-pi-pico-via-usb
   - https://blog.smittytone.net/2022/02/16/pico-usb-serial-communications-with-circuitpython
   - https://forum.micropython.org/viewtopic.php?t=11305

10. PicoW
   - https://github.com/micropython/micropython/issues/9003
   - https://stackoverflow.com/questions/74883568/oserror-errno-12-enomem-with-pi-pico-w
   - https://github.com/sekigon-gonnoc/Pico-PIO-USB/discussions/78
   - https://github.com/sekigon-gonnoc/Pico-PIO-USB/issues/76
   - https://www.nongnu.org/lwip/2_1_x/index.html
   - https://www.scaprile.com/2023/02/05/on-hardware-state-machines-how-to-write-a-simple-mac-controller-using-the-rp2040-pios
   - https://ww1.microchip.com/downloads/en/Appnotes/Atmel-42233-Using-the-lwIP-Network-Stack_AP-Note_AT04055.pdf
   - https://forums.raspberrypi.com/viewtopic.php?t=337666
   - https://forums.raspberrypi.com/viewtopic.php?t=349890

