SDK=$(HOME)/Development/tools/pico/pico-sdk
PORT=/dev/tty.usbmodem14201
SYSDATE := \\\"$(shell date '+%Y-%m-%d')\\\"
SYSTIME := \\\"$(shell date '+%H:%M:%S')\\\"
FACILITY_CODE = 49

SSID = "SHAW-1E4E80"

CLI_PORT=4242
LOG_PORT=2424
TCPD_CLIENT_IDLE=120
TCPD_SERVER_IDLE=300
TCPD_POLL_INTERVAL=500

.DEFAULT_GOAL := build

include ../../.credentials

clean:
	rm -rf build
	mkdir -p build
	export PICO_SDK_PATH=$(SDK) && \
	       cd build && \
	       cmake -DPICO_BOARD=pico_w -DSSID="$(SSID)"  -DPASSWORD="$(PASSWORD)" \
	             -DTCPD_CLI_PORT="$(CLI_PORT)" \
	             -DTCPD_LOG_PORT="$(LOG_PORT)" \
	             -DTCPD_CLIENT_IDLE="$(TCPD_CLIENT_IDLE)" \
	             -DTCPD_SERVER_IDLE="$(TCPD_SERVER_IDLE)" \
	             -DTCPD_POLL_INTERVAL="$(TCPD_POLL_INTERVAL)" \
	             ..

format: src/*.c include/*.h
	clang-format -i ../../base/src/*.c
	clang-format -i ./src/*.c
	clang-format -i ./include/*.h

build: format
	export PICO_SDK_PATH=$(SDK) && cd build && make SYSDATE=$(SYSDATE) SYSTIME=$(SYSTIME) FACILITY_CODE=$(FACILITY_CODE)

install: build
	picotool load build/wiegand-controller-wifi.uf2
	picotool reboot

run: build
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg -c "program build/wiegand-controller-wifi.elf verify reset exit"

tty:
	minicom -b 115200 -o -D $(PORT)
