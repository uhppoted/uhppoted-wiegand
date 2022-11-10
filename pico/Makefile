SDK=$(HOME)/Development/tools/pico/pico-sdk

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && cd build && cmake ..

format: test.c
	clang-format -i *.c

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make

install: build
	picotool load build/blink.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/blink.elf verify reset  exit"
