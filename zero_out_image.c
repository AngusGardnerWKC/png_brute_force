#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>


//Angus Gardner
//To complement the CRC bruteforcer, you can use this to zero out the dimensions of a PNG specified via argv


uint32_t litleE_to_bigE(uint32_t dimension){
	//https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
	uint32_t swapped = ((dimension>>24)&0xff) | // move byte 3 to byte 0
                    ((dimension<<8)&0xff0000) | // move byte 1 to byte 2
                    ((dimension>>8)&0xff00) | // move byte 2 to byte 1
                    ((dimension<<24)&0xff000000); // byte 0 to byte 3
                return swapped;
}

int main(int argc, char *argv[])
{

	if (argc != 2)
	{	
		printf("Usage:%s <PNG file path>\n",argv[0]);
		return -1;
	}	

	int fd = open(argv[1], O_RDWR | O_CREAT);

	struct stat desc;
	fstat(fd, &desc);
	char *data = mmap(NULL, desc.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(data == MAP_FAILED) {
    	perror("mmap failed");
    	exit(2);
	}

	uint32_t width = 0;
	uint32_t height = 0;

	width = litleE_to_bigE(width);
	height = litleE_to_bigE(height);

	//Set dimensions to 0
	memcpy(data+16, (char*)(&width), 4);
	memcpy(data+20, (char*)(&height), 4);

	printf("Success!\n");

	return 0;
}