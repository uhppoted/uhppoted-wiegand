.PHONY: clean
.PHONY: format

clean:
	
format: 

build-all: format
	cd base && make clean && make build
	cd usb  && make clean && make build

tty:
	 minicom -b 115200 -o -D $(PORT)
