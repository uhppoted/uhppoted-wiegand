SDK ?= $(HOME)/Development/tools/pico/pico-sdk
PORT ?= /dev/tty.usbmodem14201
BUILD ?= Debug
VERSION ?= 0x0000

.PHONY: format

clean: 
	@echo "--- $(BUILD) build ---" | tr '[:lower:]' '[:upper:]'
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && \
           cd build && \
           cmake -DCMAKE_BUILD_TYPE=$(BUILD) \
           	     -DVERSION=$(VERSION)       \
                 ..

format: 
	clang-format -i src/*.c

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make

# release: BUILD=Release clean build

run: build
	picotool load -v build/wiegand.uf2 -f

reboot: 
	picotool reboot --vid 0x2e8a --pid 0x0003 -f

tty:
	minicom -b 115200 -o -D $(PORT)
