PORT = /dev/tty.usbmodem142201
# PORT = /dev/cu.usbmodem142201

flash:
	esptool --chip esp32s3 --port $(PORT) erase-flash
	esptool --chip esp32s3 --port $(PORT) write_flash -z 0x0000 ~/Development/tools/m5/uiflow-e986468-esp32s3-spiram-16mb-cores3-v2.2.8-20250606.bin   

run:
	ampy --port $(PORT) put main.py

tty:
	minicom -b 115200 -o -D $(PORT)


