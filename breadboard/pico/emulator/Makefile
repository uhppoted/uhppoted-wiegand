clean:
    
format: common/src/*.c
	clang-format -i common/src/*.c

build-all: format
	cd pico  && make build-all
	cd picow && make build-all

tty:
	minicom -b 115200 -o -D $(PORT)
