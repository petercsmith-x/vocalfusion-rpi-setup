#!/usr/bin/env bash

# Go to script location
pushd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null
rpi_setup_dir=$(pwd -P)

i2s_mode=
xmos_device=
max_install_attempts=3
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
        "The device-type is the XMOS device to set up. Valid types: $devices_display_string" >&2
}

if [[ $# -eq 1 ]]; then
    xmos_device=$1

    # Check if the input device is valid and output usage if not
    if [[ ! " ${valid_xmos_devices[@]} " =~ " $xmos_device " ]]; then
        echo "ERROR: $xmos_device is not a valid device type." >&2
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
        echo "ERROR: Unknown XMOS device type $xmos_device." >&2
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

# Install required packages
for ((attempt=1; attempt <= max_install_attempts; attempt++)); do
    # Attempt to install all packages
    if sudo apt-get install -y ${packages[*]}; then
        break
    fi

    # Increasingly sleep between attempts
    if [[ ! $attempt -eq $max_install_attempts ]]; then
        sleep $((attempt * 2))
    fi
done

# Check for failed package installations
for package in ${packages[@]}; do
    if ! dpkg -s $package &>/dev/null; then
        failed_packages+=($package)
    fi
done

# Output failed packages
if [[ ${#failed_packages[@]} -gt 0 ]]; then
    echo "ERROR: Failed to install the following packages after $max_install_attempts attempts:" >&2
    printf ' - %s\n' "${failed_packages[@]}" >&2
    echo "Please check network connection and package names, then try again." >&2
    exit 1
fi


# TODO: Test and make different configurations for devices
# Install VocalFusion devicetree overlay
make -C $rpi_setup_dir/overlays install

# Enable VocalFusion devicetree overlay now
sudo dtoverlay vocalfusion-device

# TODO: Check out what this does
# Copy the udev rules files if device is UA
if [[ -n "$usb_mode" ]]; then
  echo "Add UDEV rules for XMOS devices"
  sudo cp $rpi_setup_dir/resources/99-xmos.rules /etc/udev/rules.d/
fi

# Move existing files to back up
if [[ -e ~/.asoundrc ]]; then
  chmod a+w ~/.asoundrc
  cp ~/.asoundrc ~/.asoundrc.bak
fi

if [[ -e /usr/share/alsa/pulse-alsa.conf ]]; then
  sudo mv /usr/share/alsa/pulse-alsa.conf  /usr/share/alsa/pulse-alsa.conf.bak
fi

# Check XMOS device for asoundrc selection.
if [[ -z $asoundrc_template ]]; then
  echo "ERROR: sound card config not known for XMOS device $xmos_device." >&2
  exit 1
fi

# Apply changes
sudo /etc/init.d/alsa-utils restart

if [[ -n $i2s_mode ]]; then
  # Create the script to run after each reboot and make the soundcard available
  i2s_setup_script=$rpi_setup_dir/resources/setup_i2s_${i2s_mode}.sh
  rm -f $i2s_setup_script

  # Sometimes with Buster on RPi3 the SYNC bit in the I2S_CS_A_REG register is not set before the drivers are loaded
  # According to section 8.8 of https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
  # this bit is set after 2 PCM clocks have occurred.
  # To avoid this issue we add a 1-second delay before the drivers are loaded
  echo "sleep 1"  >> $i2s_setup_script

  echo "# Run Alsa at startup so that alsamixer configures" >> $i2s_setup_script	
  echo "arecord -d 1 > /dev/null 2>&1"                      >> $i2s_setup_script	
  echo "aplay dummy > /dev/null 2>&1"                       >> $i2s_setup_script

  if [[ "$i2s_mode" = "master" ]]; then
    echo "# Preconfigure i2s clocks to 48kHz"               >> $i2s_setup_script
    # wait a bit as it doesn't work otherwise, this is probably caused
    # by the same process that is deleting .asoundrc
    echo "sleep 15"                                         >> $i2s_setup_script
    echo "arecord -Dhw:sndrpisimplecar,0 -c2 -fS32_LE -r48000 -s1 /dev/null" >> $i2s_setup_script
    echo "sudo $rpi_setup_dir/resources/clk_dac_setup/setup_bclk" >> $i2s_setup_script
  fi
fi

if [[ -n "$io_exp_and_dac_setup" ]]; then
  # Build setup binary
  make -C $rpi_setup_dir/resources/clk_dac_setup/

  # Create DAC and CLK setup script
  dac_and_clks_script=$rpi_setup_dir/resources/init_dac_and_clks.sh
  rm -f $dac_and_clks_script

  # Configure the clocks only if RaspberryPi is configured as I2S master
  if [[ "$i2s_mode" = "master" ]]; then
    echo "sudo $rpi_setup_dir/resources/clk_dac_setup/setup_mclk"   >> $dac_and_clks_script
    echo "sudo $rpi_setup_dir/resources/clk_dac_setup/setup_bclk"   >> $dac_and_clks_script
  fi
  
  # Note that only the substring xvfXXXX from $xmos_device is used in the lines below
  echo "python $rpi_setup_dir/resources/clk_dac_setup/setup_io_exp_and_dac.py $(echo $xmos_device | cut -c1-7)" >> $dac_and_clks_script
  echo "python $rpi_setup_dir/resources/clk_dac_setup/reset_xvf.py $(echo $xmos_device | cut -c1-7)" >> $dac_and_clks_script
fi

if [[ -n "$io_exp_and_dac_setup" ]]; then
  audacity_script=$rpi_setup_dir/resources/run_audacity.sh
  rm -f $audacity_script

  echo "#!/usr/bin/env bash" >> $audacity_script
  echo "/usr/bin/audacity &" >> $audacity_script
  echo "sleep 5" >> $audacity_script
  if [[ "$i2s_mode" = "master" ]]; then
    echo "sudo $rpi_setup_dir/resources/clk_dac_setup/setup_bclk >> /dev/null" >> $audacity_script
  fi
  sudo chmod +x $audacity_script

  # /usr/local/bin before /usr/bin in PATH
  sudo mv $audacity_script /usr/local/bin/audacity
fi

# Regenerate crontab file with new commands
crontab_file=$rpi_setup_dir/resources/crontab
if [ -n "$usb_mode" ]; then
    crontab_file="${crontab_file}_usb"
elif [ -n "$i2s_mode" ]; then
    crontab_file="${crontab_file}_i2s_${i2s_mode}"
fi

rm -f $crontab_file

# Setup the crontab to restart I2S at reboot
if [ -n "$i2s_mode" ] || [ -n "$io_exp_and_dac_setup" ]; then
  if [[ -n "$i2s_mode" ]]; then
    echo "@reboot sh $i2s_setup_script" >> $crontab_file
  fi
  if [[ -n "$io_exp_and_dac_setup" ]]; then
    echo "@reboot sh $dac_and_clks_script" >> $crontab_file
  fi
popd > /dev/null
fi

# Setup the crontab to copy the .asoundrc file at reboot
# Delay the action to allow the host to boot up
# This is needed to address the known issue in Raspian Buster:
# https://forums.raspberrypi.com/viewtopic.php?t=295008
echo "@reboot sleep 20 && cp $asoundrc_template ~/.asoundrc" >> $crontab_file

# Update crontab
crontab $crontab_file

echo "To enable all interfaces, this Raspberry Pi must be rebooted."

