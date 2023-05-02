SDK=$(HOME)/Development/tools/pico/pico-sdk
PORT=/dev/tty.usbmodem14201
SYSDATE := \\\"$(shell date '+%Y-%m-%d')\\\"
SYSTIME := \\\"$(shell date '+%H:%M:%S')\\\"
FACILITY_CODE = 49

include .credentials

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && cd build && cmake -DPICO_BOARD=pico_w -DSSID="$(SSID)"  -DPASSWORD="$(PASSWORD)" ..

format: src/*.c include/*.h
	clang-format -i ./src/*.c
	clang-format -i ./src/tcpd/*.c
	clang-format -i ./include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make SYSDATE=$(SYSDATE) SYSTIME=$(SYSTIME) FACILITY_CODE=$(FACILITY_CODE)

install: build
	picotool load build/wiegand-emulator-w.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand-emulator-w.elf verify reset exit"

tty:
	minicom -b 115200 -o -D $(PORT)
