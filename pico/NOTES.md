# NOTES

1. PIO interrupts
   - https://forums.raspberrypi.com/viewtopic.php?t=312516
   - https://forums.raspberrypi.com/viewtopic.php?t=326979

2. USB + WiFi
   - https://forums.raspberrypi.com/viewtopic.php?t=340270

3. CYW43 sysled - invoking from a repeating timer callback seems to cause a lockup
   if invoked at intervals less than 500ms.

4. 10058339
   0 0110 0100 1110 0100 0001 1111 1
     6    4    e    4    1    f
     start parity: 6+0 ones (e)
     end parity:   6+1 ones (o)

   00c9c83f
   xxxx xx00 1100 1001 1100 1000 0011 1111
   00 1100 1001 1100 1000 0011 1111
   0 0110 0100 1110 0100 0001 1111 1

   Even parity:
   0 0110 0100 1110 0100 0001 1111 1
   1 1111 1111 1111 0000 0000 0000 0
   0000 0011 1111 1111 1110 0000 0000 0000
   0    3    f    f    e    0    0    0

   odd parity:
   0 0110 0100 1110 0100 0001 1111 1
   0 0000 0000 0000 1111 1111 1111 1
   0000 0000 0000 0000 0001 1111 1111 1111
   0    0    0    0    1    f    f    f

5. https://www.raspberrypi.com/news/how-to-add-ethernet-to-raspberry-pi-pico/

### Debugging reader glitch
```
2023-01-06 13:39:50  DEBUG: 21 264255
0x0004083F

0000 0000 0100 0000 1000 0011 1111


10058399
64e41f
0110 0100 1110 0100 0001 1111
0 0110 0100 1110 0100 0001 1111 1
00 1100 1001 1100 1000 0011 1111
0000 0000 1100 1001 1100 1000 0011 1111
0    0    c     9   c    8    3     f


     0000 0000 0100 0000 1000 0011 1111
```