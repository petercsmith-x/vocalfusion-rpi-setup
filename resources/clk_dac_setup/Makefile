CFLAGS ?= -Wall -Wextra -Werror -pedantic -pthread

.PHONY: all
all: setup_mclk

setup_mclk: setup_mclk.c
	gcc ${CFLAGS} -o setup_mclk setup_mclk.c -lpigpio -lrt

.PHONY: clean
clean:
	rm -f setup_mclk
