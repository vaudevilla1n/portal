CFLAGS = -Wall -Wextra -Wpedantic -g3 -std=c23

SRC = $(wildcard src/*.c)
HDR = $(wildcard src/*.h)

.PHONY: all clean

all: portal

portal: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o portal

clean:
	rm -f ./portal
