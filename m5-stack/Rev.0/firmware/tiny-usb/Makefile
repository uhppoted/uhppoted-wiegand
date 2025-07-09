SDK ?= $(HOME)/Development/tools/pico/pico-sdk
PORT ?= /dev/tty.usbmodem142101
RPI ?= philw
BUILD ?= Debug
VERSION ?= v0.0
KEYPAD = 4-bit
FACILITY_CODE = 100
CARD_IO6 = 10085399
CARD_IO7 = 10085400

.PHONY: format

clean: 
	@echo "--- $(BUILD) build ---" | tr '[:lower:]' '[:upper:]'
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && \
           cd build && \
           cmake -DCMAKE_BUILD_TYPE=$(BUILD) \
           -DVERSION=$(VERSION) \
           -DKEYPAD=$(KEYPAD) \
           -DFACILITY_CODE=$(FACILITY_CODE) \
           -DCARD_IO6=$(CARD_IO6) \
           -DCARD_IO7=$(CARD_IO7) \
           ..

format: 
	clang-format -i src/*.c
	clang-format -i ../core/src/*.c ../core/include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make

# release: BUILD=Release clean build

run: build
	picotool load -v build/wiegand.uf2 -f

reboot: 
	picotool reboot --vid 0x2e8a --pid 0x0003 -f

tty:
	minicom -b 115200 -o -D $(PORT)

remote-run: build
	scp build/wiegand.uf2 $(RPI):/tmp
	ssh $(RPI) "picotool load -v /tmp/wiegand.uf2 -F"
	ssh $(RPI) "picotool reboot"

reboot-remote: 
	ssh $(RPI) "picotool reboot --vid 0xcafe --pid 0x4002 -f -u"

