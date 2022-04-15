# build an executable named gpiod-test and gpio-event
CC ?= gcc

gpiod-test: gpiod-test.c
	$(CC) -lgpiod -o gpiod-test gpiod-test.c

gpio-event: gpio-event-test.c
	$(CC) -o gpio-event  gpio-event-test.c

lsram-counter-test: lsram-counter-test.c
	$(CC) -O2 -o lsram-counter-test lsram-counter-test.c

all:  gpiod-test gpio-event lsram-counter-test
clean:
	rm -rf gpiod-test gpio-event lsram-counter-test .*.swp .*.un* *~
