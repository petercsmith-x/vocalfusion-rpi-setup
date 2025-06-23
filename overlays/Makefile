.PHONY: all
all: dummy-xmos-device.dtbo

dummy-xmos-device.dtbo: dummy-xmos-device.dts
	dtc -@ -I dts -O dtb -o dummy-xmos-device.dtbo dummy-xmos-device.dts

install: dummy-xmos-device.dtbo
	sudo cp dummy-xmos-device.dtbo /boot/overlays
	if ! grep -q '^dtoverlay=dummy-xmos-device$$' /boot/config.txt; then \
		echo "dtoverlay=dummy-xmos-device" | sudo tee -a /boot/config.txt; \
	fi

uninstall:
	sudo sed -i '/dtoverlay=dummy-xmos-device/d' /boot/config.txt
	sudo rm /boot/overlays/dummy-xmos-device.dtbo
	sudo cp dummy-xmos-device.dtbo ${RPI_CONFIG_ROOT}/overlays
	if ! grep -q '^dtoverlay=dummy-xmos-device$$' ${RPI_CONFIG_ROOT}/config.txt; then \
		echo 'dtoverlay=dummy-xmos-device' | sudo tee -a ${RPI_CONFIG_ROOT}/config.txt; \
	fi

uninstall:
	sudo sed -i '/dtoverlay=dummy-xmos-device/d' ${RPI_CONFIG_ROOT}/config.txt
	-sudo rm ${RPI_CONFIG_ROOT}/overlays/dummy-xmos-device.dtbo

clean:
	rm *.dtbo
