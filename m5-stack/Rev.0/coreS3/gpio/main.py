import os
import sys
import io
import M5
import time

from collections import namedtuple
from M5 import *
from machine import Pin
from utility import print_error_msg

Point = namedtuple("Point", "x y timestamp")
Button = namedtuple("Button", "x y w h label pin")

BLACK = 0x222222
BACKGROUND = 0xEACDCD
BORDER = 0xFC0000

IO1 = Pin(1, Pin.OUT)
IO2 = Pin(2, Pin.OUT)
B10058399 = Button(32, 32, 256, 48, "10058399", IO2)
B10058400 = Button(32, 112, 256, 48, "10058400", IO1)

touched = None


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


def button(b):
    Widgets.Rectangle(b.x, b.y, b.w, b.h, BORDER, BACKGROUND)
    Widgets.Label(b.label, b.x + 80, b.y + 14, 1.0, BLACK, BACKGROUND, Widgets.FONTS.DejaVu18)


def pressed(b):
    global touched

    return touched.x >= b.x and touched.x <= (b.x + b.w) and touched.y >= b.y and touched.y <= (b.y + b.h):


def press(b):
    if b.pin.value():
        b.pin.value(False)
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


def loop():
    global IO1, IO2, touched
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
                    if press(B10058399):
                        Speaker.tone(880, 125)  # A5
                        print(f"pressed  {B10058399.label}")

                if pressed(B10058400):
                    if press(B10058400):
                        Speaker.tone(1046, 125)  # C6'ish
                        print(f"pressed  {B10058400.label}")
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
