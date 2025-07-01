SDK ?= $(HOME)/Development/tools/pico/pico-sdk
PORT ?= /dev/tty.usbmodem142101
BUILD ?= Debug
VERSION ?= v0.0
FACILITY_CODE = 49
KEYPAD = 4-bit

.PHONY: format

clean: 
	@echo "--- $(BUILD) build ---" | tr '[:lower:]' '[:upper:]'
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && \
           cd build && \
           cmake -DCMAKE_BUILD_TYPE=$(BUILD) -DVERSION=$(VERSION) -DKEYPAD=$(KEYPAD) -DFACILITY_CODE=$(FACILITY_CODE) ..

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
