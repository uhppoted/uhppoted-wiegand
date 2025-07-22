import os
import re
import sys
import io
import M5
import time

from collections import namedtuple
from M5 import *
from machine import Pin, UART
from utility import print_error_msg

Point = namedtuple("Point", "x y timestamp")
Button = namedtuple("Button", "x y w h label pressed")
Label = namedtuple("Label", "x y w h text")

BLACK = 0x222222
BACKGROUND = 0xEACDCD
GRAY = 0x808080
DARKGRAY = 0x444444
RED = 0xC02020
BORDER = 0xFC0000

IO1 = Pin(1, Pin.OUT)
IO2 = Pin(2, Pin.OUT)
uart1 = UART(1, baudrate=115200, tx=43, rx=44)
uart2 = UART(2, baudrate=115200, tx=17, rx=18)

B10058399 = Button(32, 8, 120, 48, "10058399", lambda: press(IO2))
B10058400 = Button(32 + 136, 8, 120, 48, "10058400", lambda: press(IO1))
B10058401 = Button(32, 8 + 64, 120, 48, "10058401", lambda: swipe(uart1, "10058401", None))
C1357 = Button(32 + 136, 8 + 64, 120, 48, "1357#", lambda: keypad(uart1, "1357"))
B10058402 = Button(32, 8 + 128, 256, 48, "10058402 + 7531#", lambda: swipe(uart2, "10058402", "7531"))
LMSG = Label(0, 216, 320, 24, "")

touched = None
message = None


def setup():
    global IO1, IO2
    M5.begin()
    IO1.value(True)
    IO2.value(True)
    screen()


def screen():
    Widgets.setRotation(1)
    Widgets.fillScreen(BLACK)
    button(B10058399)
    button(B10058400)
    button(B10058401)
    button(B10058402)
    button(C1357)
    label(LMSG)


def button(b):
    offset = int(b.w / 2 - 12 * len(b.label) / 2)
    Widgets.Rectangle(b.x, b.y, b.w, b.h, BORDER, BACKGROUND)
    Widgets.Label(b.label, b.x + offset, b.y + 14, 1.0, BLACK, BACKGROUND, Widgets.FONTS.DejaVu18)


def label(l):
    global message
    Widgets.Rectangle(l.x, l.y, l.w, l.h, DARKGRAY, DARKGRAY)
    message = Widgets.Label(l.text, l.x + 16, l.y + 6, 1.0, RED, DARKGRAY, Widgets.FONTS.DejaVu12)


def pressed(b):
    global touched

    return touched.x >= b.x and touched.x <= (b.x + b.w) and touched.y >= b.y and touched.y <= (b.y + b.h)


def press(pin):
    message.setText(f"%-48s" % "")
    if pin.value():
        pin.value(False)
        message.setText(f"%-48s" % "Ok")
        return True

    return False


def release():
    global IO1, IO2

    if not IO2.value():
        IO2.value(True)
        print(f"released {B10058399.label}")

    if not IO1.value():
        IO1.value(True)
        print(f"released {B10058400.label}")


def swipe(uart, card, code):
    display("")

    if code is None:
        uart.write(f"SWIPE {card}\n")
    else:
        uart.write(f"SWIPE {card} {code}#\n")

    return read(uart)


def keypad(uart, code):
    display("")
    uart.write(f"CODE {code}#\n")

    return read(uart)


def read(uart):
    timeout = 500  # milliseconds
    start = time.ticks_ms()
    response = b""

    while time.ticks_diff(time.ticks_ms(), start) < timeout:
        if uart.any():
            response += uart.read()
            if b"\n" in response:
                break

    if response:
        line = response.decode().strip()
        print(f">>> {line}")
        if line == "OK":
            display("Ok")
            return True

        match = re.compile(r"^ERROR\s+(\d+)\s+(.+)$").match(line)
        if match:
            code = int(match.group(1))
            msg = match.group(2)
            display(f"** ERROR {code} {msg}")
            return False

        display("** ERROR <unknown>")
        return False

    display(f"** ERROR <no reply>")
    return False


def display(msg):
    global message
    message.setText(f"%-128s" % f"{msg}")


def loop():
    global touched

    M5.update()

    if M5.Touch.getCount() > 0:
        now = time.ticks_ms()
        point = M5.Touch.getTouchPointRaw()

        if touched is None:
            touched = Point(point[0], point[1], now)
        else:
            dt = now - touched.timestamp
            dx = point[0] - touched.x
            dy = point[1] - touched.y
            touched = Point(point[0], point[1], now)

            if dt > 250 or abs(dx) > 10 or abs(dy) > 10:
                if pressed(B10058399):
                    if B10058399.pressed():
                        Speaker.tone(784, 125)  # G5

                if pressed(B10058400):
                    if B10058400.pressed():
                        Speaker.tone(698, 125)  # F5

                if pressed(B10058401):
                    if B10058401.pressed():
                        Speaker.tone(880, 125)  # A5

                if pressed(B10058402):
                    if B10058402.pressed():
                        Speaker.tone(1046, 125)  # C6'ish

                if pressed(C1357):
                    if C1357.pressed():
                        Speaker.tone(1174, 125)  # D6'ish
    else:
        release()

    time.sleep(0.05)


if __name__ == "__main__":
    try:
        setup()
        while True:
            loop()
    except (Exception, KeyboardInterrupt) as e:
        try:
            print_error_msg(e)
        except ImportError:
            print("please update to latest firmware")
