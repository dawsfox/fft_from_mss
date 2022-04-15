# build an executable named gpiod-test and gpio-event
CC ?= gcc

start_stop_test: start_stop_test.c
	$(CC) -lgpiod -o start_stop_test start_stop_test.c

lsram-counter-test: lsram-counter-test.c
	$(CC) -O2 -o lsram-counter-test lsram-counter-test.c

all:  gpiod-test gpio-event lsram-counter-test
clean:
	rm -rf gpiod-test gpio-event lsram-counter-test .*.swp .*.un* *~
