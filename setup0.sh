#!/usr/bin/env bash

# Go to script location
pushd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null
rpi_setup_dir=$(pwd -P)

i2s_mode=
xmos_device=
max_install_attempts=
valid_xmos_devices=(xvf3800-intdev xvf3800-inthost)

packages=(python3-matplotlib python3-numpy libatlas-base-dev audacity libreadline-dev libncurses-dev)

# TODO: No ua devices currently supported
# packages_ua=(libusb-1.0-0-dev libevdev-dev libudev-dev)

usage() {
    # Get devices as a nice comma separated string
    printf -v devices_display_string '%s, ' "${valid_xmos_devices[@]}"
    devices_display_string="${devices_display_string%, }"

    printf "%s\n" \
        'This script sets up the Raspberry Pi to use different XMOS devices.' \
        "USAGE: $0 <device-type>" \
        "The device-type is the XMOS device to set up. Valid types: $devices_display_string"
}

if [[ $# -eq 1 ]]; then
    xmos_device=$1

    # Check if the input device is valid and output usage if not
    if [[ ! " ${valid_xmos_devices[@]} " =~ " $xmos_device " ]]; then
        echo "ERROR: $xmos_device is not a valid device type."
        echo
        usage
        exit 1
    fi
else
    usage
    exit 1
fi

case $xmos_device in
    xvf3800-intdev)
        i2s_mode=master
        # TODO: is this needed?
        io_exp_and_dac_setup=y
        asoundrc_template=$rpi_setup_dir/resources/asoundrc_vf
        ;;
    xvf3800-intdev)
        i2s_mode=slave
        # TODO: is this needed?
        io_exp_and_dac_setup=y
        asoundrc_template=$rpi_setup_dir/resources/asoundrc_vf
        ;;
    *)
        # This shouldn't happen as we've already validated the input device
        echo "ERROR: Unknown XMOS device type $xmos_device."
        echo
        usage
        exit 1
        ;;
esac

# Begin setting up RasPi
rpi_config=/boot/config.txt

# Disable built-in audio
sudo sed -i '/^dtparam=audio=on$/ s/^/#/' "$rpi_config"

# Enable I2S devicetree
sudo sed -i '/^#dtparam=i2s=on$/s/^#//' "$rpi_config"

# Enable I2C devicetree
sudo raspi-config nonint do_i2c 0

# Set I2C baudrate to 100k
if ! grep -q '^dtparam=i2c_arm_baudrate=100000$' "$rpi_config"; then
    echo "dtparam=i2c_arm_baudrate=100000" | sudo tee -a "$rpi_config" >/dev/null
fi

# Enable SPI
sudo raspi-config nonint do_spi 0

# TODO: Test and make different configurations for devices
# Install VocalFusion devicetree overlay
make -C $rpi_setup_dir/overlays install

# Enable VocalFusion devicetree overlay now
sudo dtoverlay vocalfusion-device
