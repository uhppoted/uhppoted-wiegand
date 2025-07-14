import os, sys, io
import M5
import time

from M5 import *
from utility import print_error_msg

def setup():
  Widgets.setBrightness(128)
  Widgets.fillScreen(0x00c400)

def loop():
  print("woot")
  time.sleep(5)
  M5.update()

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
