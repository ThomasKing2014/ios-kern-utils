/*
 * kmem.c - Read kernel memory and dump it to the console
 *
 * Copyright (c) 2014 Samuel Groß
 * Copyright (c) 2016 Siguza
 */

#include <errno.h>              // errno
#include <stdbool.h>            // bool, true, false
#include <stdio.h>              // printf, fprintf
#include <stdlib.h>             // free, malloc, strtoul
#include <string.h>             // memset, strlen
#include <unistd.h>             // getopt, write, STDOUT_FILENO

#include <mach/mach_types.h>    // task_t
#include <mach/vm_types.h>      // vm_address_t, vm_size_t

#include "arch.h"               // ADDR
#include "libkern.h"            // read_kernel

static void hexdump(unsigned char *data, size_t size)
{
    int i;
    char cs[17];
    memset(cs, 0, 17);

    for(i = 0; i < size; i++)
    {
        if(i != 0 && i % 0x10 == 0)
        {
            printf(" |%s|\n", cs);
            memset(cs, 0, 17);
        }
        else if(i != 0 && i % 0x8 == 0)
        {
            printf(" ");
        }
        printf("%02X ", data[i]);
        cs[(i % 0x10)] = (data[i] >= 0x20 && data[i] <= 0x7e) ? data[i] : '.';
    }

    i = i % 0x10;
    if(i != 0)
    {
        if(i <= 0x8)
        {
            printf(" ");
        }
        while(i++ < 0x10)
        {
            printf("   ");
        }
    }
    printf(" |%s|\n", cs);
}

static void print_usage(const char *self)
{
    fprintf(stderr, "Usage: %s [-r] [-h] addr length\n"
                    "0x for hex, no prefix for decimal\n", self);
}

static void too_few_args(const char *self)
{
    fprintf(stderr, "[!] Too few arguments\n");
    print_usage(self);
}

int main(int argc, char **argv)
{
    bool raw = false; // print raw bytes instead of a hexdump
    task_t kernel_task;
    vm_address_t addr;
    vm_size_t size;
    char c, *end;

    while((c = getopt(argc, argv, "rh")) != -1)
    {
        switch (c)
        {
            case 'r':
                raw = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
        }
    }

    if(argc < optind + 2)
    {
        too_few_args(argv[0]);
        return -1;
    }

    // addr
    errno = 0;
    addr = strtoull(argv[optind], &end, 0);
    if(argv[optind] == '\0' || *end != '\0' || errno != 0)
    {
        fprintf(stderr, "[!] Failed to parse argument: %s\n", argv[optind] == '\0' ? "zero characters gives" : strerror(errno));
        return -1;
    }

    // size
    errno = 0;
    size = strtoull(argv[optind + 1], &end, 0);
    if(argv[optind + 1] == '\0' || *end != '\0' || errno != 0)
    {
        fprintf(stderr, "[!] Failed to parse argument: %s\n", argv[optind + 1] == '\0' ? "zero characters gives" : strerror(errno));
        return -1;
    }
    if(size == 0)
    {
        fprintf(stderr, "[!] Size must be > 0\n");
        return -1;
    }

    if(get_kernel_task(&kernel_task) != KERN_SUCCESS)
    {
        fprintf(stderr, "[!] Failed to get kernel task\n");
        return -1;
    }

    if(!raw)
    {
        fprintf(stderr, "[*] Reading " SIZE " bytes from 0x" ADDR "\n", size, addr);
    }
    unsigned char* buf = malloc(size);
    read_kernel(addr, size, buf);

    if(raw)
    {
        write(STDOUT_FILENO, buf, size);
    }
    else
    {
        hexdump(buf, size);
    }

    free(buf);
    return 0;
}
