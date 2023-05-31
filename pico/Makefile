SDK=$(HOME)/Development/tools/pico/pico-sdk
PORT=/dev/tty.usbmodem14201
SYSDATE := \\\"$(shell date '+%Y-%m-%d')\\\"
SYSTIME := \\\"$(shell date '+%H:%M:%S')\\\"
FACILITY_CODE = 49

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && cd build && cmake ..

format: base/src/*.c base/include/*.h
	clang-format -i base/src/*.c
	clang-format -i base/include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make SYSDATE=$(SYSDATE) SYSTIME=$(SYSTIME) FACILITY_CODE=$(FACILITY_CODE)

build-all: clean build
	cd reference           && make clean && make build
	cd reference/usb       && make clean && make build
	cd reference/wifi      && make clean && make build
	cd reference/usb+wifi  && make clean && make build
	cd emulator            && make clean && make build
	cd emulator/usb        && make clean && make build
	cd emulator/wifi       && make clean && make build
	cd emulator/usb+wifi   && make clean && make build
	cd controller          && make clean && make build
	cd controller/usb      && make clean && make build
	cd controller/wifi     && make clean && make build
	cd controller/usb+wifi && make clean && make build

install: build
	picotool load build/wiegand.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand.elf verify reset exit"

tty:
	 minicom -b 115200 -o -D $(PORT)
