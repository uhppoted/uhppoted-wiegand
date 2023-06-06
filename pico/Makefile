SDK=$(HOME)/Development/tools/pico/pico-sdk
PORT=/dev/tty.usbmodem14201
SYSDATE := \\\"$(shell date '+%Y-%m-%d')\\\"
SYSTIME := \\\"$(shell date '+%H:%M:%S')\\\"
FACILITY_CODE = 49

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && cd build && cmake ..

format: core/src/*.c core/include/*.h
	clang-format -i core/src/*.c
	clang-format -i core/src/pico/*.c
	clang-format -i core/src/picow/*.c
	clang-format -i core/include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make SYSDATE=$(SYSDATE) SYSTIME=$(SYSTIME) FACILITY_CODE=$(FACILITY_CODE)

build-all: clean build
	cd reference/pico/base       && make clean && make build
	cd reference/pico/usb        && make clean && make build
	cd reference/picow/base      && make clean && make build
	cd reference/picow/wifi      && make clean && make build
	cd reference/picow/wifi+usb  && make clean && make build

	cd emulator/pico             && make clean && make build
	cd emulator/pico/usb         && make clean && make build
	cd emulator/picow/wifi       && make clean && make build
	cd emulator/picow/wifi+usb   && make clean && make build

	cd controller/pico/base      && make clean && make build
	cd controller/pico/usb       && make clean && make build
	cd controller/picow/base     && make clean && make build
	cd controller/picow/wifi     && make clean && make build
	cd controller/picow/wifi+usb && make clean && make build

install: build
	picotool load build/wiegand.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand.elf verify reset exit"

tty:
	 minicom -b 115200 -o -D $(PORT)
