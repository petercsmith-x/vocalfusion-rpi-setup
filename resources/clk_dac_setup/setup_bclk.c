#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[argc + 1]) {
  unsigned frequency = 3072000;
  unsigned input;

  switch (argc) {
  case 1:
    break;
  case 2:
    input = atoi(argv[1]);

    if (input != 48000 && input != 16000) {
      fprintf(
          stderr,
          "ERROR: Unsupported frequency (%d), only 48kHz and 16kHz supported.",
          frequency);
      return EXIT_FAILURE;
    }

    if (input == 16000) {
        frequency = 1024000;
    }

    break;
  default:
    fprintf(stderr, "Usage: %s [clock-rate]\nDefault clock rate is 48kHz.\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  int err = 0;
  if (gpioInitialise() < 0) {
    fprintf(stderr, "ERROR: PiGPIO Failed to initialise.\n");
    return EXIT_FAILURE;
  }

  // Set GPIO19 (pin 35) to ALT0 (PCM FS)
  err = gpioSetMode(19, PI_ALT0);
  if (err) {
    fprintf(stderr, "ERROR: Failed to set GPIO19 to PCM FS.\n");

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
  err = gpioSetPullUpDown(19, PI_PUD_OFF);
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

  // Turn hardware clock on at 3.072 MHz for 48 kHz or 1.024 MHz for 16 kHz 
  err = gpioHardwareClock(19, frequency);
  if (err) {
    fprintf(stderr, "ERROR: Failed to start hardware clock on GPIO19.\n");

    switch (err) {
    case PI_BAD_GPIO:
      fprintf(stderr, "ERROR: Bad GPIO.\n");
      break;
    case PI_NOT_HCLK_GPIO:
      fprintf(stderr, "ERROR: Can't start a hardware clock on GPIO19.\n");
      break;
    case PI_BAD_HCLK_FREQ:
      fprintf(stderr,
              "ERROR: Can't start hardware clock at this frequency (%d).\n",
              frequency);
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
