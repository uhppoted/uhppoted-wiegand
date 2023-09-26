SDK ?= $(HOME)/Development/tools/pico/pico-sdk

PORT=/dev/tty.usbmodem14201
SYSDATE := \\\"$(shell date '+%Y-%m-%d')\\\"
SYSTIME := \\\"$(shell date '+%H:%M:%S')\\\"
FACILITY_CODE = 49
MASTER_PASSCODE=54321

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && \
	       cd build && \
	       cmake -DMASTER_PASSCODE="$(MASTER_PASSCODE)" ..

format: src/*.c include/*.h
	clang-format -i ./src/*.c
	clang-format -i ./include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make clean && make SYSDATE=$(SYSDATE) SYSTIME=$(SYSTIME) FACILITY_CODE=$(FACILITY_CODE)

install: build
	picotool load build/wiegand-emulator-usb.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand-controller-usb.elf verify reset exit"

tty:
	minicom -b 115200 -o -D $(PORT)
