# NOTES

1. PIO interrupts
   - https://forums.raspberrypi.com/viewtopic.php?t=312516
   - https://forums.raspberrypi.com/viewtopic.php?t=326979

2. 10058339
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