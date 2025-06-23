# xCORE VocalFusion Raspberry Pi Setup

This repository provides a simple-to-use automated script to configure the Raspberry Pi to use **xCORE VocalFusion** for audio.

This setup will perform the following operations:

- enable the I2S, I2C and SPI interfaces
- update the Raspberry Pi's packages
- install the required packages
- install a devicetree overlay for I2S
- update the asoundrc file to support I2S devices
- add a cron job to load the I2S drivers at boot up

For XVF361x-INT devices these actions will be done as well:

- configure MCLK at 12288kHz from pin 7 (BCM 4)
- configure I2S BCLK at 3072kHz from pin 12 (BCM 18)
- update the alias for Audacity
- update the asoundrc file to support I2S devices
- add a cron job to reset the device at boot up
- add a cron job to configure the DAC at boot up

For XVF3800 devices these actions will be done as well:

- configure I2S BCLK at 3072kHz from pin 12 (BCM 18)
- update the alias for Audacity
- update the asoundrc file to support I2S devices
- add a cron job to reset the device at boot up
- add a cron job to configure the IO expander at boot up

<!-- For XVF3800-extmclk devices these actions will be done as well: -->
<!-- - configure MCLK at 12288kHz from pin 7 (BCM 4) and drive to XVF3800 -->

<!-- For XVF3510-UA and XVF361x-UA devices these actions will be done as well: -->
<!---->
<!-- - update the asoundrc file to support USB devices -->
<!-- - update udev rules so that root privileges are not needed to access USB control interface -->

## Setup

1. First, install the Raspberry Pi imager on a host computer. This is available [here](https://www.raspberrypi.org/software) or through your package manager.
   
   Windows:
   ```powershell
   winget install --id=RaspberryPiFoundation.RaspberryPiImager -e
   ```

   Ubuntu:
   ```bash
   sudo apt install rpi-imager
   ```

   Run the imager (may require `root` privileges), select the Raspberry Pi you are using (tested on Pi 4<!-- TODO: Pi 3, Pi 5-->), Bookworm and Bullseye are both supported.

   Then, choose your SD card and write to it. When prompted, remove the SD card and insert it into the Raspberry Pi.

2. Connect peripherals (keyboard, mouse, speakers/headphones, and display recommended), and power up the system. Refer to the [Getting Started Guide](https://www.raspberrypi.com/documentation/computers/getting-started.html) for your Raspberry Pi.

   Set up locale, username, password, and networking.

3. On the Raspberry Pi, clone this GitHub repository:

   ```bash
   git clone https://github.com/xmos/vocalfusion-rpi-setup
   ```

4. Simply run the setup script for your device. Run `./setup -h` for usage.

   For example, for an XVF3800 with 48kHz sample rate:
   ```bash
   ./setup.sh xvf3800
   ```

   You will be prompted to restart your Raspberry Pi, this will apply options modified in the Raspberry Pi `config.txt` and any ALSA configuration.

## Important note on clocks

> [!TIP]
> The setup script provides the `-r` flag to configure this for you. You can simply run the script again with a different sample rate and reboot.

The I2S/PCM driver that is provided with raspbian does not support an MCLK output. However the 
driver does have full ability to set the BCLK and LRCLK correctly for a given sample rate. As 
the driver does not know about the MCLK it is likely to choose dividers for the clocks generators
which are not phase locked to the MCLK. The script in this repo gets around this problem by 
configuring the I2S driver to a certain frequency and then overriding the clock registers to force
a phase locked frequency.

This will work until a different sample rate is chosen by an application, then the I2S driver will
write it's own value to the clocks and the MCLK will no longer be phase locked. To solve this problem
the following steps must be taken before connecting an XVF device with a different sample rate:

1. Take a short recording at the new sample rate: `arecord -c2 -fS32_LE -r{sample_rate} -s1 -Dhw:sndrpisimplecar`
2. For 48kHz `./resources/clk_dac_setup/setup_blk`, for 16kHz `./resources/clk_dac_setup/setup_blk 16000`
