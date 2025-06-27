CFLAGS ?= -Wall -Wextra -Werror -pedantic -pthread

.PHONY: all
all: setup_bclk setup_mclk

setup_mclk: setup_mclk.c
	gcc ${CFLAGS} -o setup_mclk setup_mclk.c -lpigpio -lrt

setup_bclk: setup_bclk.c
	gcc ${CFLAGS} -o setup_bclk setup_bclk.c -lpigpio -lrt

.PHONY: clean
clean:
	rm -f setup_mclk
	rm -f setup_bclk
