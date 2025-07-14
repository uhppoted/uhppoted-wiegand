import os, sys, io
import M5
import time

from M5 import *
from utility import print_error_msg

B10058400 = None
T10058400 = None

def setup():
  global B10058400, T10058400

  M5.begin()
  Widgets.setRotation(1)
  Widgets.fillScreen(0x222222)
  B10058400 = Widgets.Rectangle(32, 32, 256, 32, 0xfc0000, 0xeacdcd)
  T10058400 = Widgets.Label("10058400", 112, 38, 1.0, 0x222222, 0xeacdcd, Widgets.FONTS.DejaVu18)

  # page0.screen_load()

def loop():
  global B10058400, T10058400
  M5.update()
  (w, x, y, z) = M5.Touch.getTouchPointRaw()
  print(f'w:{w} x:{x} y:{y} z:{z}')
  time.sleep(1)

if __name__ == '__main__':
  try:
    setup()
    while True:
      loop()
  except (Exception, KeyboardInterrupt) as e:
    try:
      print_error_msg(e)
    except ImportError:
      print("please update to latest firmware")