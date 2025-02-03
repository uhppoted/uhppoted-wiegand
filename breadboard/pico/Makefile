clean:

format: core/src/*.c core/include/*.h
	clang-format -i core/src/*.c
	clang-format -i core/src/pico/*.c
	clang-format -i core/src/picow/*.c
	clang-format -i core/include/*.h

build: format

build-all: clean build
	@echo ">>>>>>>>>>>>>> BUILD-ALL <$(CREDENTIALS)>"
	# cd universal  && make clean && make build-all
	# cd controller && make clean && make build-all
	# cd emulator   && make clean && make build-all

tty:
	 minicom -b 115200 -o -D $(PORT)
