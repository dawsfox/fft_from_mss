# build an executable named gpiod-test and gpio-event
CC ?= gcc

start_stop_test: start_stop_test.c
	$(CC) -lgpiod -o start_stop_test start_stop_test.c

lsram-counter-test: lsram-counter-test.c
	$(CC) -O2 -o lsram-counter-test lsram-counter-test.c

axi_master_test: axi_master_test.c
	$(CC) -lgpiod -o axi_master_test axi_master_test.c

all:  gpiod-test gpio-event lsram-counter-test axi_master_test
clean:
	rm -rf gpiod-test gpio-event lsram-counter-test axi_master_test .*.swp .*.un* *~
