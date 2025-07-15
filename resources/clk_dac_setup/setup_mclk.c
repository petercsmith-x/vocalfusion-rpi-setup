// Copyright (c) 2019-2025, XMOS Ltd, All rights reserved

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Base addresses for different Pi models
static volatile uint32_t piPeriphBase = 0x20000000;

#define CLK_BASE (piPeriphBase + 0x101000)
#define GPIO_BASE (piPeriphBase + 0x200000)

#define CLK_LEN 0xA8
#define GPIO_LEN 0xB4

// Clock control register bits
#define CLK_PASSWD (0x5A << 24)
#define CLK_CTL_MASH(x) ((x) << 9)
#define CLK_CTL_BUSY (1 << 7)
#define CLK_CTL_KILL (1 << 5)
#define CLK_CTL_ENAB (1 << 4)
#define CLK_CTL_SRC(x) ((x) << 0)

// Clock sources
#define CLK_CTL_SRC_OSC 1  /* 19.2 MHz */
#define CLK_CTL_SRC_PLLC 5 /* 1000 MHz */
#define CLK_CTL_SRC_PLLD 6 /* 500 MHz for RPi 3, 750 MHz for RPi 4*/
#define CLK_CTL_SRC_HDMI 7 /* 216 MHz */

// Clock divider macros
#define CLK_DIV_DIVI(x) ((x) << 12)
#define CLK_DIV_DIVF(x) ((x) << 0)

// GPCLK0 registers (GPIO4)
#define CLK_GP0_CTL 28
#define CLK_GP0_DIV 29

// Hardware revision codes
#define HW_REVISION_RPI2 0x01
#define HW_REVISION_RPI3 0x02
#define HW_REVISION_RPI4 0x03

// GPIO modes
// https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
// Table 6.2
#define PI_INPUT 0
#define PI_OUTPUT 1
#define PI_ALT0 4

static volatile uint32_t *gpioReg = MAP_FAILED;
static volatile uint32_t *clkReg = MAP_FAILED;

void gpioSetMode(unsigned gpio, unsigned mode) {
  // https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
  // Section 6.2
  // Each GPIO function select register controls 10 pins
  int reg = gpio / 10;
  // Each pin uses 3 bits for mode selection
  int shift = (gpio % 10) * 3;

  // Clear and set new mode on pin
  gpioReg[reg] = (gpioReg[reg] & ~(7 << shift)) | (mode << shift);
}

unsigned getGpioHardwareRevision(void) {
  static unsigned rev = 0;
  FILE *fp;
  char buf[512];
  char term;

  if (rev) {
    return rev;
  }

  fp = fopen("/proc/cpuinfo", "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open /proc/cpuinfo\n");
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    // Look for a line like:
    // Revision    : c03114
    if (!strncasecmp("revision\t:", buf, 10)) {
      // Put the hex value into rev
      if (sscanf(buf + 10, "%x%c", &rev, &term) == 2) {
        // Make sure the terminator is a new line
        if (term != '\n') {
          rev = 0;
        }
      }
    }
  }
  fclose(fp);

  // The format of this is specified here:
  // https://github.com/raspberrypi/documentation/blob/develop/documentation/asciidoc/computers/raspberry-pi/revision-codes.adoc
  // We want to select the processor, though the revision may also work

  // Original rev is something like:
  // 0b110000000011000100010100
  // After shift:
  // 0b110000000011
  // After mask:
  // 0b11 (HW_REVISION_RPI4)
  return (rev >> 12) & 0xF;
}

static int setupMCLK(int source_clk_khz, int target_freq_hz, int enable) {
  // Calculate dividers for target frequency
  // Formula: output_clock = source_clock / (divI + divF / 4096)
  double divisor = (double)source_clk_khz * 1000.0 / target_freq_hz;
  int divI = (int)divisor;
  int divF = (int)((divisor - divI) * 4096);

  // Validate divider values
  if (divI < 2 || divI > 4095) {
    fprintf(stderr, "Invalid integer divider: %d\n", divI);
    return -1;
  }

  if (divF < 0 || divF > 4095) {
    fprintf(stderr, "Invalid fractional divider: %d\n", divF);
    return -1;
  }

  printf("Setting MCLK to %.3f kHz using PLLD (I=%d F=%d)\n",
         (float)source_clk_khz / (divI + (float)divF / 4096), divI, divF);

  // Stop the clock first
  clkReg[CLK_GP0_CTL] = CLK_PASSWD | CLK_CTL_KILL;

  // Wait for clock to stop
  while (clkReg[CLK_GP0_CTL] & CLK_CTL_BUSY) {
    usleep(10);
  }

  // Set the divider
  clkReg[CLK_GP0_DIV] = CLK_PASSWD | CLK_DIV_DIVI(divI) | CLK_DIV_DIVF(divF);
  usleep(10);

  // Configure clock source (PLLD) and MASH filter
  clkReg[CLK_GP0_CTL] =
      CLK_PASSWD | CLK_CTL_MASH(1) | CLK_CTL_SRC(CLK_CTL_SRC_PLLD);
  usleep(10);

  // Enable clock if requested
  if (enable) {
    clkReg[CLK_GP0_CTL] |= CLK_PASSWD | CLK_CTL_ENAB;
  }

  return 0;
}

static uint32_t *initMapMem(int fd, uint32_t addr, uint32_t len) {
  return (uint32_t *)mmap(0, len, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_SHARED | MAP_LOCKED, fd, addr);
}

int gpioInitialise(void) {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    fprintf(stderr, "This program needs root privileges. Try using sudo\n");
    return -1;
  }

  // Map both the GPIO and CLK registers
  gpioReg = initMapMem(fd, GPIO_BASE, GPIO_LEN);
  clkReg = initMapMem(fd, CLK_BASE, CLK_LEN);
  close(fd);

  if (gpioReg == MAP_FAILED || clkReg == MAP_FAILED) {
    fprintf(stderr, "Memory mapping failed\n");
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[argc + 1]) {
  unsigned hw_rev = getGpioHardwareRevision();
  unsigned source_clk_khz;
  int disable_mode = 0;

  // Check for disable flag
  if (argc > 1 && !strcmp(argv[1], "--disable")) {
    disable_mode = 1;
    printf("Disabling MCLK output\n");
  }

  // Set peripheral base address based on Pi model
  switch (hw_rev) {
  case HW_REVISION_RPI2:
  case HW_REVISION_RPI3:
    printf("Raspberry Pi 2/3 detected\n");
    piPeriphBase = 0x3F000000;
    source_clk_khz = 500000; // 500 MHz PLLD
    break;

  case HW_REVISION_RPI4:
    printf("Raspberry Pi 4 detected\n");
    piPeriphBase = 0xFE000000;
    source_clk_khz = 750000; // 750 MHz PLLD
    break;

  default:
    fprintf(stderr, "Unsupported hardware revision code (0x%x)\n", hw_rev);
    return EXIT_FAILURE;
  }

  if (gpioInitialise() < 0) {
    return EXIT_FAILURE;
  }

  if (disable_mode) {
    // Set GPIO4 to input mode to disable clock output
    gpioSetMode(4, PI_INPUT);
    printf("MCLK disabled\n");
  } else {
    // Set up MCLK at 12.288 MHz
    if (setupMCLK(source_clk_khz, 12288000, 1) < 0) {
      fprintf(stderr, "Error setting up MCLK\n");
      return EXIT_FAILURE;
    }

    // Set GPIO4 to ALT0 mode (GPCLK0)
    gpioSetMode(4, PI_ALT0);
    printf("MCLK enabled on GPIO4 (pin 7)\n");
  }

  return EXIT_SUCCESS;
}
