#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int err = 0;
  if (gpioInitialise() < 0) {
    fprintf(stderr, "ERROR: PiGPIO Failed to initialise.\n");
    return EXIT_FAILURE;
  }

  // Set GPIO4 (pin 7) to ALT0 (GPCLK0)
  err = gpioSetMode(4, PI_ALT0);
  if (err) {
    fprintf(stderr, "ERROR: Failed to set GPIO4 to GPCLK0.\n");

    if (err == PI_BAD_GPIO) {
      fprintf(stderr, "ERROR: Bad GPIO.\n");
    } else if (err == PI_BAD_MODE) {
      fprintf(stderr, "ERROR: Bad mode.\n");
    } else {
      fprintf(stderr, "ERROR: Unknown error %d.\n", err);
    }

    return EXIT_FAILURE;
  }

  // Turn pull-up/down resistors off
  err = gpioSetPullUpDown(4, PI_PUD_OFF);
  if (err) {
    fprintf(stderr, "ERROR: Failed to set pull-up/down resistors.\n");

    if (err == PI_BAD_GPIO) {
      fprintf(stderr, "ERROR: Bad GPIO.\n");
    } else if (err == PI_BAD_PUD) {
      fprintf(stderr, "ERROR: Bad pull-up/down.\n");
    } else {
      fprintf(stderr, "ERROR: Unknown error %d.\n", err);
    }

    return EXIT_FAILURE;
  }

  // Turn hardware clock on at 12.288 MHz
  err = gpioHardwareClock(4, 12288000);
  if (err) {
    fprintf(stderr, "ERROR: Failed to start hardware clock on GPIO4.\n");

    switch (err) {
    case PI_BAD_GPIO:
      fprintf(stderr, "ERROR: Bad GPIO.\n");
      break;
    case PI_NOT_HCLK_GPIO:
      fprintf(stderr, "ERROR: Can't start a hardware clock on GPIO4.\n");
      break;
    case PI_BAD_HCLK_FREQ:
      fprintf(
          stderr,
          "ERROR: Can't start hardware clock at this frequency (12.288MHz).\n");
      break;
    case PI_BAD_HCLK_PASS:
      fprintf(stderr, "ERROR: Bad hardware clock pass.\n");
      break;
    default:
      fprintf(stderr, "ERROR: Unknown error.\n");
      break;
    }

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
