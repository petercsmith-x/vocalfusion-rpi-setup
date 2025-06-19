.PHONY: all
all: vocalfusion-device.dtbo

vocalfusion-device.dtbo: vocalfusion-device.dts
	dtc -@ -I dts -O dtb -o vocalfusion-device.dtbo vocalfusion-device.dts

install: vocalfusion-device.dtbo
	sudo cp vocalfusion-device.dtbo /boot/overlays
	if ! grep -q '^dtoverlay=vocalfusion-device$$' /boot/config.txt; then \
		echo "dtoverlay=vocalfusion-device" | sudo tee -a /boot/config.txt; \
	fi

uninstall:
	sudo sed -i '/dtoverlay=vocalfusion-device/d' /boot/config.txt
	sudo rm /boot/overlays/vocalfusion-device.dtbo

clean:
	rm vocalfusion-device.dtbo
