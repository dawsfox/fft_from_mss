// SPDX-License-Identifier: MIT
/*
 *	GPIOD example -- blink PolarFire SoC Icicle kit LEDs and read SW2 switch state
 *
 *	Copyright (c) 2021 Microchip Inc.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include <gpiod.h>

#define led_GPIO16 16
#define led_GPIO17 17
#define led_GPIO18 18
#define led_GPIO19 19
#define sw2_GPIO30 30

/* libgpiod public API */
/* Reference: https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/ */
/**
 * @brief Open a gpiochip by number.
 * @param num Number of the gpiochip.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine appends num to '/dev/gpiochip' to create the path.
 */
//struct gpiod_chip *gpiod_chip_open_by_number(unsigned int num) GPIOD_API;


/**
 * @brief Get the handle to the GPIO line at given offset.
 * @param chip The GPIO chip object.
 * @param offset The offset of the GPIO line.
 * @return Pointer to the GPIO line handle or NULL if an error occured.
 */
//struct gpiod_line *
//gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) GPIOD_API;

/**
 * @brief Set the value of a single GPIO line.
 * @param line GPIO line object.
 * @param value New value.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
//int gpiod_line_set_value(struct gpiod_line *line, int value) GPIOD_API;

/**
 * @brief Read current value of a single GPIO line.
 * @param line GPIO line object.
 * @return 0 or 1 if the operation succeeds. On error this routine returns -1
 *         and sets the last error number.
 */
//int gpiod_line_get_value(struct gpiod_line *line) GPIOD_API;

	  

int main()
{
	char *chipname = "/dev/gpiochip0";
	struct gpiod_chip *chip;
	int counter = 0;

	chip = gpiod_chip_open(chipname);
	if (!chip) {
		perror("Open chip failed\n");
		goto end;
	}
  
	struct gpiod_line *fft_start;
	struct gpiod_line *fft_done;
	// accompanying FPGA implementation has changed GPIO2 bit 17
	// from LED2 to a start signal for the FFT block
	// first check done; should start low
	fft_done = gpiod_chip_get_line(chip, sw2_GPIO30);
	gpiod_line_request_input(fft_done, "fft_done");
	printf("fft_done on start: %d\n", gpiod_line_get_value(fft_done));

	fft_start = gpiod_chip_get_line(chip, led_GPIO17);
	if (!fft_start) {
		perror("fft_start Get line failed\n");
		goto close_chip;
	}
	/* config as output and set a description */
	gpiod_line_request_output(fft_start, "fft_start", GPIOD_LINE_ACTIVE_STATE_HIGH);
	gpiod_line_set_value(fft_start, 1);
	printf("fft_start signal set HIGH\n");
	
	int value = -1;
	while (value != 1) {
		value = gpiod_line_get_value(fft_done);
		sleep(1);
	}
	printf("fft_done signal read HIGH\n");
	// release line holds
	gpiod_line_release(fft_start);
	gpiod_line_release(fft_done);
	printf("\n\t Exiting GPIO Example program...  \n");
	goto close_chip;
close_chip:   
   gpiod_chip_close(chip);
 end:  
   return 0;  
   
}
