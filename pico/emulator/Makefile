clean:
    
format: common/src/*.c
	clang-format -i common/src/*.c

build-all: format
	cd pico/base      && make clean && make build
	cd pico/usb       && make clean && make build
	cd picow/base     && make clean && make build
	cd picow/wifi     && make clean && make build
	cd picow/wifi+usb && make clean && make build

tty:
	minicom -b 115200 -o -D $(PORT)
