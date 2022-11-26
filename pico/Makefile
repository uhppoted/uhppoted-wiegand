SDK=$(HOME)/Development/tools/pico/pico-sdk
PORT=/dev/tty.usbmodem14201
SYSDATE := $(shell date '+%Y-%m-%d')
SYSTIME := $(shell date '+%H:%M:%S')

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && cd build && cmake ..

format: src/*.c include/*.h
	clang-format -i src/*.c
	clang-format -i include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make SYSDATE=\\\"$(SYSDATE)\\\" SYSTIME=\\\"$(SYSTIME)\\\"

install: build
	picotool load build/wiegand.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand.elf verify reset exit"

tty:
	 minicom -b 115200 -o -D $(PORT)
