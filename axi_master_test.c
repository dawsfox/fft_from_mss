// SPDX-License-Identifier: MIT
/*
 * FPGA fabric lsram example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <gpiod.h>

#define SYSFS_PATH_LEN		(128)
#define ID_STR_LEN		(32)
#define UIO_DEVICE_PATH_LEN	(32)
#define NUM_UIO_DEVICES		(32)

#define fft_done_GPIO30 30
#define axi_error_GPIO31 31
#define mss_start_GPIO17 17
#define mss_reset_GPIO18 18

char uio_id_str[] = "fpga_lsram";
char sysfs_template[] = "/sys/class/uio/uio%d/%s";

/* returns value of CSR counting cycles since startup */
static inline uint32_t rdcyc() {
	uint32_t val;
	asm volatile ("rdcycle %0 ;\n":"=r" (val) ::);
	return val;
}

/* returns value of CSR counting instructions retired since startup */
static inline uint32_t rdret() {
	uint32_t val;
	asm volatile ("rdinstret %0 ;\n":"=r" (val) ::);
	return val;
}

static inline void mem_fence() {
	asm volatile ("fence;\n");
	return;
}


/*****************************************/
/* function get_uio_device will return */
/* the uio device number */
/*************************************** */

int get_uio_device(char * id)
{
    FILE *fp;
    int i;
    int len;
    char file_id[ID_STR_LEN];
    char sysfs_path[SYSFS_PATH_LEN];

    for (i = 0; i < NUM_UIO_DEVICES; i++) {
        snprintf(sysfs_path, SYSFS_PATH_LEN, sysfs_template, i, "/name");
        fp = fopen(sysfs_path, "r");
        if (fp == NULL)
            break;

        fscanf(fp, "%32s", file_id);
        len = strlen(id);
        if (len > ID_STR_LEN-1)
            len = ID_STR_LEN-1;
        if (strncmp(file_id, id, len) == 0) {
	    //debug
	    printf("file id: %s\tsysfs_path: %s\n", file_id, sysfs_path);
            return i;
        }
    }

    return -1;
}

/*****************************************/
/* function get_memory_size will return */
/* the uio device size */
/*************************************** */

uint32_t get_memory_size(char *sysfs_path, char *uio_device)
{
    FILE *fp;
    uint32_t sz;

    /* 
     * open the file the describes the memory range size.
     * this is set up by the reg property of the node in the device tree
     */
    fp = fopen(sysfs_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "unable to determine size for %s\n", uio_device);
        exit(0);
    }

    fscanf(fp, "0x%016X", &sz);
    fclose(fp);
    return sz;
}

int main(int argc, char* argvp[])
{
    int retCode = 0;
    int uioFd_0,index=0;
    char uio_device[UIO_DEVICE_PATH_LEN];
    char sysfs_path[SYSFS_PATH_LEN], d1;
    volatile uint32_t *mem_ptr0;
    uint32_t mmap_size;
    uint32_t val = 0;
    volatile uint32_t i =0 ;

    printf("locating device for %s\n", uio_id_str);
        index = get_uio_device(uio_id_str);
    if (index < 0) {
        fprintf(stderr, "can't locate uio device for %s\n", uio_id_str);
        return -1;
    }

    snprintf(uio_device, UIO_DEVICE_PATH_LEN, "/dev/uio%d", index);
    printf("located %s\n", uio_device);

    uioFd_0 = open(uio_device, O_RDWR);
    if(uioFd_0 < 0) {
        fprintf(stderr, "cannot open %s: %s\n", uio_device, strerror(errno));
        return -1;
    } else {
        printf("opened %s (r,w)\n", uio_device);
    }

    snprintf(sysfs_path, SYSFS_PATH_LEN, sysfs_template, index, "maps/map0/size");
    mmap_size = get_memory_size(sysfs_path, uio_device);
    printf("mmap_size = %d\n", mmap_size);
    if (mmap_size == 0) {
        fprintf(stderr, "bad memory size for %s\n", uio_device);
        return -1;
    }

    while(1) {
        printf("\n\t # Choose one of  the following options: \n\t Enter 1 to perform LSRAM test \n\t Enter 2 to perform AXI Master to LSRAM test \n\t Enter 3 to Exit\n");
        scanf("%c%*c",&d1);
        if(d1=='3'){
            break;
        }else if(d1=='1'){
            mem_ptr0 = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_0, 0*getpagesize());
            if(mem_ptr0 == MAP_FAILED){
                fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
                close(uioFd_0);
                return -1;
            }
            printf("\nSize of memory at 0x61000000 is 0x%x\n", mmap_size);





	    uint64_t cyc_beg = rdcyc();
	    
            for(i=0;i<(mmap_size/4);i++) {
                *(mem_ptr0+i)=i;
	        mem_fence();
            }
	    
	    //mem_fence();
	    //val = *(mem_ptr0+1023);
	    uint64_t cyc_mid = rdcyc();
            for(i=0;i<(mmap_size/4);i++) {
                val=*(mem_ptr0+i);
            }
	    mem_fence();
	    uint64_t cyc_end = rdcyc();
	    printf("Wrote and read %d elements\n", (mmap_size/4));
	    printf("%u cycles for writing\n", (cyc_mid-cyc_beg));
	    printf("%u cycles for reading\n", (cyc_end-cyc_mid));

        }else if(d1=='2'){
	    
	    char *chipname = "/dev/gpiochip0";
	    struct gpiod_chip *chip;
	    chip = gpiod_chip_open(chipname);
	    if (!chip) {
		perror("Open chip failed\n");
		goto end;
	    }
  
	    struct gpiod_line *fft_start;
	    struct gpiod_line *fft_done;
	    struct gpiod_line *axi_error;
	    struct gpiod_line *mss_reset;
	    fft_done = gpiod_chip_get_line(chip, fft_done_GPIO30);
	    axi_error = gpiod_chip_get_line(chip, axi_error_GPIO31);
	    gpiod_line_request_input(fft_done, "fft_done");
	    gpiod_line_request_input(axi_error, "axi_error");
	    printf("fft_done on start: %d\n", gpiod_line_get_value(fft_done));
	    printf("axi_error on start: %d\n", gpiod_line_get_value(axi_error));

	    fft_start = gpiod_chip_get_line(chip, mss_start_GPIO17);
	    mss_reset = gpiod_chip_get_line(chip, mss_reset_GPIO18);
	    if (!fft_start) {
		perror("fft_start Get line failed\n");
   		gpiod_chip_close(chip);
		goto end;
	    }
	    if (!mss_reset) {
		perror("mss_reset Get line failed\n");
		gpiod_chip_close(chip);
		goto end;
	    }
	    /* config as output and set a description */
            printf("\nSignaling AXI master to write\n");
	    gpiod_line_request_output(fft_start, "fft_start", GPIOD_LINE_ACTIVE_STATE_HIGH);
	    gpiod_line_request_output(mss_reset, "mss_reset", GPIOD_LINE_ACTIVE_STATE_HIGH);
	    // first set reset high
	    gpiod_line_set_value(mss_reset, 1);
	    sleep(1); //delay
	    // lower reset signal
	    gpiod_line_set_value(mss_reset, 0); 
	    // signal to start fft
	    gpiod_line_set_value(fft_start, 1);
	
	    int value = -1;
	    int counter = 0;
	    while (value != 1 && counter < 5) {
		value = gpiod_line_get_value(fft_done);
		sleep(1);
		counter++;
	    }
	    printf("fft_done signal read HIGH or ERROR\n");
	    //check error signal
	    printf("axi_error after loop: %d\n", gpiod_line_get_value(axi_error));
	    // now reset start signal
	    gpiod_line_set_value(fft_start, 0);

            mem_ptr0 = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_0, 0*getpagesize());
            if(mem_ptr0 == MAP_FAILED){
                fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
                close(uioFd_0);
                return -1;
            }
	    /*
            for(i=0;i<(mmap_size/4);i++) {
                *(mem_ptr0+i)=i;
	        mem_fence();
            }
	    */
            printf("\nReading data starting from address 0x61000000 \n");
	    unsigned first_test = *(mem_ptr0);
	    unsigned second_test = *(mem_ptr0+1);
	    unsigned third = *(mem_ptr0+2);
	    unsigned fourth = *(mem_ptr0+3);
	    printf("first four of LSRAM: %d\t%d\t%d\t%d\n", first_test, second_test, third, fourth);
	    gpiod_line_release(fft_start);
	    gpiod_line_release(fft_done);
	    gpiod_line_release(axi_error);
	
        }else {
            printf("Enter either 1, 2, or 3\n");
        }
end:
        retCode = munmap((void*)mem_ptr0, mmap_size);
        printf("unmapped %s\n", uio_device);
    }

    retCode = close(uioFd_0);
    printf("closed %s\n", uio_device);
    return retCode;
}
