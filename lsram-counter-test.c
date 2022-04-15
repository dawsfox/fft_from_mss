// SPDX-License-Identifier: MIT
/*
 * FPGA fabric lsram example for the Microchip PolarFire SoC
 *
 * Copyright (c) 2021 Microchip Technology Inc. All rights reserved.
 */

#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#define SYSFS_PATH_LEN		(128)
#define ID_STR_LEN		(32)
#define UIO_DEVICE_PATH_LEN	(32)
#define NUM_UIO_DEVICES		(32)

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
        printf("\n\t # Choose one of  the following options: \n\t Enter 1 to perform memory test on LSRAM \n\t Enter 2 to Exit\n");
        scanf("%c%*c",&d1);
        if(d1=='2'){
            break;
        }else if(d1=='1'){
            mem_ptr0 = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_0, 0*getpagesize());
            if(mem_ptr0 == MAP_FAILED){
                fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
                close(uioFd_0);
                return -1;
            }
            printf("\nSize of memory at 0x61000000 is 0x%x\n", mmap_size);
            printf("\nWriting incremental pattern starting from address 0x61000000\n");
            printf("\nReading data starting from address 0x61000000 \n");
            printf("\nComparing data \n");

	    printf("Linear access:\n");
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
		/*
                if(val !=i){
                    printf("\n\n\r ***** LSRAM memory test Failed***** \n\r");
                    break;
                }
                else if(i%100 == 0)
                {
                    printf(".");
                }                
		*/
            }
	    mem_fence();
	    uint64_t cyc_end = rdcyc();
	    printf("Wrote and read %d elements\n", (mmap_size/4));
	    printf("%u cycles for writing\n", (cyc_mid-cyc_beg));
	    printf("%u cycles for reading\n", (cyc_end-cyc_mid));

	    /* Same reads and writes but with stride for testing 
            printf("Stride of 64:\n");
            int j;
	    cyc_beg = rdcyc();
	    for(j=0; j<64; j++){
                for(i=j;i<(mmap_size/4);i=i+64) {
                    *(mem_ptr0+i)=i;
	            mem_fence();
                }
	    }
	    //mem_fence();
	    cyc_mid = rdcyc();
	    for(j=0; j<64; j++) {
                for(i=j;i<(mmap_size/4);i=i+64) {
                    val=*(mem_ptr0+i);
                }
	    }
	    mem_fence();
	    cyc_end = rdcyc();

	    printf("Wrote and read %d elements\n", (mmap_size/4));
	    printf("%u cycles for writing\n", (cyc_mid-cyc_beg));
	    printf("%u cycles for reading\n", (cyc_end-cyc_mid));
          



            if(i == (mmap_size/4))
                printf("\n\n\n**** LSRAM memory test passed successfully *****\n");
            */
        }else if(d1=='3'){
            mem_ptr0 = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, uioFd_0, 0*getpagesize());
            if(mem_ptr0 == MAP_FAILED){
                fprintf(stderr, "Cannot mmap: %s\n", strerror(errno));
                close(uioFd_0);
                return -1;
            }
            for(i=0;i<(mmap_size/4);i++) {
                *(mem_ptr0+i)=i;
	        mem_fence();
            }
	
        }else {
            printf("Enter either 1 or 2\n");
        }
        retCode = munmap((void*)mem_ptr0, mmap_size);
        printf("unmapped %s\n", uio_device);
    }

    retCode = close(uioFd_0);
    printf("closed %s\n", uio_device);
    return retCode;
}
