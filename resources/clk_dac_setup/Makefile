CFLAGS ?= -Wall -Wextra -Werror -pedantic -pthread

.PHONY: all
all: setup_mclk

setup_bclk: setup_bclk.c
	gcc ${CFLAGS} -o setup_bclk setup_bclk.c -lpigpio -lrt

.PHONY: clean
clean:
	rm -f setup_mclk
