#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include "pngcrc.h"
#define pass (void)0

/*

Angus Gardner, 14/05/2021

The actual CRC computation comes from https://gist.github.com/minatu2d/a1bd400adb41312dd842
The code from the github gist features source code from libpng, which is incredibly cool. 

My brute forcer will use this code to compute the png CRC, instead of running the pngcheck program on it.
Because, spawning a new process is so much slower than running a function, so this is the only acceptable
approach to brute forcing.

There's absolutely no way I'd have been able to re-write the crc computing bit...
*/

//
// Base on http://www.libpng.org/pub/png/spec/1.2/PNG-Structure.html
//
#define PNG_SIGNATURE (8)	
#define PNG_LENGTH_FIELD_SIZE (4)
#define CHUNK_TYPE_SIZE (4)
#define CRC_SIZE (4)
#define PNG_FILE_MIN_SIZE (PNG_LENGTH_FIELD_SIZE + CHUNK_TYPE_SIZE + PNG_SIGNATURE)


//
// Read a unsigned int in binary file
//
static unsigned int readUint(FILE *fi)
{
	unsigned int val = 0;
	unsigned char oneBytes[4];
	if (fread(&oneBytes[0], 1, 4, fi) == 4)
	{
		//printf("%x%x%x%x\n",oneBytes[0], oneBytes[1], oneBytes[2], oneBytes[3]);
		val += oneBytes[0] * (1 << 24);
		val += oneBytes[1] * (1 << 16);
		val += oneBytes[2] * (1 << 8);
		val += oneBytes[3] * (1 << 0);
	}	
	else 
	{
		//printf("End of file HERE");
	}
	return val;
}

//
// Read a int in binary file
//
static int readint(FILE *fi)
{
	int val = 0;
	unsigned char oneBytes[4];
	if (fread(&oneBytes[0], 1, 4, fi) == 4)
	{
		//printf("%x%x%x%x\n",oneBytes[0], oneBytes[1], oneBytes[2], oneBytes[3]);
		val += oneBytes[0] * (1 << 24);
		val += oneBytes[1] * (1 << 16);
		val += oneBytes[2] * (1 << 8);
		val += oneBytes[3] * (1 << 0);
	}	
	else 
	{
		//printf("End of file HERE");
	}
	return val;
}

static int chunkCount = 0;

//
// CRC in each chunk
//
int crc_in_chunk(FILE *fi)
{
	int ret = 0;

	if (feof(fi))
	{
		return 1;
	}
	
	// Read chunk length
	unsigned int chunkLength = 	readUint(fi);
	//printf("Chunklength after readUint is: %d\n", chunkLength);
	
	unsigned int chunkSize = chunkLength + CHUNK_TYPE_SIZE;
	if (!chunkLength)// No data
	{
		// Read over CRC
		readint(fi);// Read ChunkType
		readint(fi);// Read CRC
		ret = 1;
	}
	else{
		unsigned char *chunkDatPtr = (unsigned char*) malloc(sizeof(unsigned char) * chunkSize);
		unsigned int nread = fread(chunkDatPtr, sizeof(unsigned char), chunkSize, fi);
	if (chunkSize == nread)
	{
		unsigned int realCRC = (unsigned int)crc(chunkDatPtr, chunkSize);
		unsigned int infileCRC = readint(fi);
		if (realCRC == infileCRC)
		{
			ret = 1;
		}
		else 
		{
			ret = 0;
		}
	}
	else 
	{
		
		ret = 0;
	}
	free(chunkDatPtr);
}
	
	
	
	return ret;
}


/*Funky little to big endian swap for cool people
	There's actually some neat "__bswap_32" for this, 
	but this is more portable. Not like I made this for 
	portability after using mmap over malloc though...
	but I wanted to use syscalls to achieve it, and
	the less of a wrapper there is around a syscall (like using malloc),
	the more time it will save in the long run. So technically
	I do have a compelling argument for using mmap over malloc*/
uint32_t litleE_to_bigE(uint32_t dimension){
	//https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
	uint32_t swapped = ((dimension>>24)&0xff) | // move byte 3 to byte 0
                    ((dimension<<8)&0xff0000) | // move byte 1 to byte 2
                    ((dimension>>8)&0xff00) | // move byte 2 to byte 1
                    ((dimension<<24)&0xff000000); // byte 0 to byte 3
                return swapped;
}



    //memcpy magic to move the now big endian value 0x10 into the png.
    /*https://en.wikipedia.org/wiki/Portable_Network_Graphics
    Believe it or not, the wikipedia page is top tier for this, because it 
    straight up tells you the file signature, and what bytes mean what in the 
    file.*/

    //Also this is dead easy in Python using slice[] but it's crazy slow because Python.
void writeWidth(char *data, uint32_t width){
	uint32_t swapped = litleE_to_bigE(width);
	memcpy(data+16, (char*)(&swapped), 4);
}

void writeHeight(char *data, uint32_t height){
	uint32_t swapped = litleE_to_bigE(height);
	memcpy(data+20, (char*)(&swapped), 4);
}



int main(int argc, char* argv[])
{
	
	if (argc != 6)
	{
		printf("%d", argc);
		printf("Usage:%s <PNG file path> <starting width> <starting height> <ending width (ceiling)> ending height (ceiling)\n",argv[0]);
		return -1;
	}

	long StartingWidth = strtol(argv[2], NULL, 10);
	long StartingHeight = strtol(argv[3], NULL, 10);
	long EndWidth = strtol(argv[4], NULL, 10);
	long EndHeight = strtol(argv[5], NULL, 10);

	//Lets dive in with file descriptors. New thing for me, but pretty cool huh
	int fd = open(argv[1], O_RDWR | O_CREAT);

	struct stat desc;
	fstat(fd, &desc);
	//And mmap takes a file descriptor and returns a pointer. Kinda like malloc() but less
	//portable, but cooler and closer to the actual syscall.
	char *data = mmap(NULL, desc.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//Always gotta have those checks
	if(data == MAP_FAILED) {
    	perror("mmap failed");
    	exit(2);
	}	

	/*
	After attempting to re-write the fread implementation to instead use memcpy, I ended up reading in the buffer
	as a file stream. Since the original code reads from a file pointer, it's easier to implement, but possibly sacrifices
	some speed becase I had to use fseek()
	*/
	//Return the buffer as a file pointer.
	FILE* fi = fmemopen(data, desc.st_size, "r");

	//fseek back to 8 bytes into the file
	fseek(fi, PNG_SIGNATURE, SEEK_SET);
	int ret = 0;


	for (uint32_t width=StartingWidth; width<=EndWidth; width++){
		for(uint32_t height=StartingHeight; height<=EndHeight; height++){
			printf("Checking dimensions: %dx%d\n", width, height);
			writeWidth(data, width);
			writeHeight(data, height);
			if(crc_in_chunk(fi) !=1){
				//Seek back to 8 bytes into the file.
				fseek(fi, PNG_SIGNATURE, SEEK_SET);
				pass;
			}
			else{
				//We'll chop 1 off of the height, because it's already added in this instance of the for loop.
				printf("Found correct dimensions! Width: %d, Height: %d\n", width, height-1);
				writeHeight(data, height-1);
				exit(0);
					}
				}
			}
		fclose(fi);

	return 0;
}
