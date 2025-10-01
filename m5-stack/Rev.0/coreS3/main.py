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

PAGES = 2

IO1 = Pin(1, Pin.OUT)
IO2 = Pin(2, Pin.OUT)
uart1 = UART(1, baudrate=115200, tx=43, rx=44)
uart2 = UART(2, baudrate=115200, tx=17, rx=18)

BTNA = Button(64,  248, 64, 32, "A", lambda: left())
BTNB =  Button(192, 248, 64, 32, "B", lambda: right())

B10058399 = Button(32, 8, 120, 48, "10058399", lambda: press(IO2))
B10058400 = Button(32 + 136, 8, 120, 48, "10058400", lambda: press(IO1))
B10058401 = Button(32, 8 + 64, 120, 48, "10058401", lambda: swipe(uart1, "10058401", None))
C1357 = Button(32 + 136, 8 + 64, 120, 48, "1357#", lambda: keypad(uart1, "1357#"))
B10058402 = Button(32, 8 + 128, 256, 48, "10058402 + 7531#", lambda: swipe(uart2, "10058402", "7531#"))
LMSG = Label(0, 216, 320, 24, "")

KEY1 = Button(20,    1, 52, 44, "1", lambda: keypad(uart1, '1'))
KEY2 = Button(88,    1, 52, 44, "2", lambda: keypad(uart1, '2'))
KEY3 = Button(156,   1, 52, 44, "3", lambda: keypad(uart1, '3'))
KEY4 = Button(20,   55, 52, 44, "4", lambda: keypad(uart1, '4'))
KEY5 = Button(88,   55, 52, 44, "5", lambda: keypad(uart1, '5'))
KEY6 = Button(156,  55, 52, 44, "6", lambda: keypad(uart1, '6'))
KEY7 = Button(20,  110, 52, 44, "7", lambda: keypad(uart1, '7'))
KEY8 = Button(88,  110, 52, 44, "8", lambda: keypad(uart1, '8'))
KEY9 = Button(156, 110, 52, 44, "9", lambda: keypad(uart1, '9'))
STAR = Button(20,  165, 52, 44, "*", lambda: keypad(uart1, '*'))
KEY0 = Button(88,  165, 52, 44, "0", lambda: keypad(uart1, '0'))
HASH = Button(156, 165, 52, 44, "#", lambda: keypad(uart1, '#'))
SWIPE = Button(220, 16, 88, 64, "SWIPE",lambda: swipe(uart1, "10058400", None))

touched = None
message = None
page = 1

page1 = [ B10058399, B10058400, B10058401, B10058402, C1357 ]
page2 = [ KEY1, KEY2, KEY3, KEY4, KEY5, KEY6, KEY7, KEY8, KEY9, STAR, KEY0, HASH, SWIPE ]

def setup():
    global IO1, IO2
    M5.begin()
    IO1.value(True)
    IO2.value(True)
    screen()
    # Power.setExtOutput(True)

def screen():
    global page

    Widgets.setRotation(1)
    Widgets.fillScreen(BLACK)
    label(LMSG)

    if page == 1:
        for b in page1:
            button(b)

    if page == 2:
        for b in page2:
            button(b)

def button(b):
    xoffset = int(b.w / 2 - 12 * len(b.label) / 2)
    yoffset = int(b.h / 2 - 6)
    Widgets.Rectangle(b.x, b.y, b.w, b.h, BORDER, BACKGROUND)
    Widgets.Label(b.label, b.x + xoffset, b.y + yoffset, 1.0, BLACK, BACKGROUND, Widgets.FONTS.DejaVu18)


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
    uart.write(f"CODE {code}\n")

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
        line = response.replace(b'\r', b'').decode().strip()
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


def left():
    global page
    print("LEFT")
    if page > 1:
        page = page - 1
        screen()

def right():
    global page
    print("RIGHT")
    if page < PAGES:
        page = page + 1
        screen()

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
                if pressed(BTNA):
                    BTNA.pressed()

                if pressed(BTNB):
                    BTNB.pressed()

                if page == 1 and pressed(B10058399):
                    if B10058399.pressed():
                        Speaker.tone(784, 125)  # G5

                if page == 1 and pressed(B10058400):
                    if B10058400.pressed():
                        Speaker.tone(698, 125)  # F5

                if page == 1 and pressed(B10058401):
                    if B10058401.pressed():
                        Speaker.tone(880, 125)  # A5

                if page == 1 and pressed(B10058402):
                    if B10058402.pressed():
                        Speaker.tone(1046, 125)  # C6'ish

                if page == 1 and pressed(C1357):
                    if C1357.pressed():
                        Speaker.tone(1174, 125)  # D6'ish

                if page == 2:
                    for b in page2:
                        if pressed(b):
                            if b.pressed():
                                Speaker.tone(784, 125)  # G5
                                break


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
