.PHONY: all clean

all: portal

portal:
	make -C src/

clean:
	rm -f ./portal
