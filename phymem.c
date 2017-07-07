// ref: https://stackoverflow.com/questions/12040303/accessing-physical-address-from-user-space

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>

/*
 * -f	path
 * -a	address
 * -s	size
 * -w	width
 */

typedef struct PhyMem PhyMem;
struct PhyMem
{
	char path[PATH_MAX];
	size_t address;
	size_t size;
	size_t width;

	unsigned char* mapped_address;
	size_t mapped_size;
	off_t offset;
};

void __getopt(int argc, char* argv[], PhyMem* phy_mem)
{
	int opt;

	// default
	strcpy(phy_mem->path, "/dev/mem");
	phy_mem->address = 0x0;
	phy_mem->size = 0x4;
	phy_mem->width = 0x4;

    while ((opt = getopt(argc, argv, "f:a:s:w:")) != -1) {
        switch (opt) {
        case 'f':
			strcpy(phy_mem->path, optarg);
            break;
        case 'a':
            phy_mem->address = strtoull(optarg, NULL, 0);
            break;
        case 's':
            phy_mem->size = strtoull(optarg, NULL, 0);
            break;
        case 'w':
            phy_mem->width = strtoull(optarg, NULL, 0);
            break;
        default:
            fprintf(stderr, "Usage: %s [-a physical_address] [-s size]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

void __map(PhyMem* phy_mem)
{
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    off_t page_base = (phy_mem->address / pagesize) * pagesize;
    phy_mem->offset = phy_mem->address - page_base;
    phy_mem->mapped_size = phy_mem->offset + phy_mem->size;

    int fd = open(phy_mem->path, O_SYNC);
	if(fd == -1)
	{
		perror("open()");
		exit(EXIT_FAILURE);
	}
    phy_mem->mapped_address = mmap(NULL, phy_mem->mapped_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, page_base);
    if (phy_mem->mapped_address == MAP_FAILED)
	{
        perror("mmap()");
		exit(EXIT_FAILURE);
    }
	close(fd);
}

void __unmap(PhyMem* phy_mem)
{
	if(munmap(phy_mem->mapped_address, phy_mem->mapped_size) == -1)
	{
		perror("munmap()");
		exit(EXIT_FAILURE);
	}
}

#define DUMP(addr, offset, len, type, format)											\
do																						\
{ 																						\
	size_t i;																			\
																						\
    for (i = 0; i < len / sizeof(type); i++)											\
	{																					\
		type* ptr = (type*)&(addr[offset + i * sizeof(type)]);							\
		printf("0x%p: " format "\n", ptr, *ptr);										\
	}																					\
}while(0);																				\

void __dump(PhyMem* phy_mem)
{
	switch(phy_mem->width)
	{
		case 1:
			DUMP(phy_mem->mapped_address, phy_mem->offset, phy_mem->size, uint8_t, "0x%02x");
			break;
		case 2:
			DUMP(phy_mem->mapped_address, phy_mem->offset, phy_mem->size, uint16_t, "0x%04x");
			break;
		case 4:
			DUMP(phy_mem->mapped_address, phy_mem->offset, phy_mem->size, uint32_t, "0x%08x");
			break;
		default:
            fprintf(stderr, "Invalid width\n");
            exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[])
{
	PhyMem phy_mem;

	__getopt(argc, argv, &phy_mem);
	__map(&phy_mem);
	__dump(&phy_mem);
	__unmap(&phy_mem);

    return EXIT_SUCCESS;
}
