.PHONY: all clean

all: portal
	make -C src/

clean:
	rm -f ./portal
