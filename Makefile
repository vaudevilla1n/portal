CFLAGS = -Wall -Wextra -Wpedantic -g3 -std=c23

.PHONY: all clean

all: portal

portal: portal.c

clean:
	rm -f ./portal
