RPI_CONFIG_ROOT ?= /boot

.PHONY: all
all: dummy-xmos-device.dtbo mclk-overlay-pi5.dtbo

dummy-xmos-device.dtbo: dummy-xmos-device.dts
	dtc -@ -I dts -O dtb -o dummy-xmos-device.dtbo dummy-xmos-device.dts

mclk-overlay-pi5.dtbo: mclk-overlay-pi5.dts
	dtc -@ -I dts -O dtb -o mclk-overlay-pi5.dtbo mclk-overlay-pi5.dts

install: dummy-xmos-device.dtbo
	sudo cp dummy-xmos-device.dtbo ${RPI_CONFIG_ROOT}/overlays
	if ! grep -q '^dtoverlay=dummy-xmos-device$$' ${RPI_CONFIG_ROOT}/config.txt; then \
		echo 'dtoverlay=dummy-xmos-device' | sudo tee -a ${RPI_CONFIG_ROOT}/config.txt; \
	fi

setup-mclk-rpi5: mclk-overlay-pi5.dtbo
	sudo cp mclk-overlay-pi5.dtbo ${RPI_CONFIG_ROOT}/overlays
	if ! grep -q '^dtoverlay=mclk-overlay-pi5$$' ${RPI_CONFIG_ROOT}/config.txt; then \
		echo 'dtoverlay=mclk-overlay-pi5' | sudo tee -a ${RPI_CONFIG_ROOT}/config.txt; \
	fi

uninstall:
	sudo sed -i '/dtoverlay=dummy-xmos-device/d' ${RPI_CONFIG_ROOT}/config.txt
	sudo sed -i '/dtoverlay=mclk-overlay-pi5/d' ${RPI_CONFIG_ROOT}/config.txt
	-sudo rm ${RPI_CONFIG_ROOT}/overlays/dummy-xmos-device.dtbo
	-sudo rm ${RPI_CONFIG_ROOT}/overlays/mclk-overlay-pi5.dtbo

clean:
	rm *.dtbo
