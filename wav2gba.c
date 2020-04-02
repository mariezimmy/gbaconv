/*
 * Forked from https://github.com/fcambus/gbaconv
 * Includes writing .c and .h files with sound array information
 * to the directory where the provided .wav resides
 */

#include <ctype.h> 
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>  

#define WAVE_HEADER_LENGTH 44

char *input_file_buffer;


struct wave_header {
	char chunk_ID[4];
	unsigned int chunk_size;
	char format[4];
	char fmt_chunk[4];
	unsigned int fmt_chunk_size;
	unsigned short int audio_format;
	unsigned short int channels;
	unsigned int sample_rate;
	unsigned int byte_rate;
	unsigned short int block_align;
	unsigned short int bits_per_sample;
	char data_chunk[4];
	unsigned int data_chunk_size;
} wave_header;

int main(int argc, char *argv[])
{
	int fd;
	struct stat st;

	// capitalize the input name
	char* name = malloc(strlen(argv[2]));
	strcat(name, argv[2]);
	int i = 0;
	while(name[i]) {
		name[i] = toupper(name[i]);
		i++;
	}

	// create .c file name from input
	char* cFile = malloc(strlen(argv[1]) - 2);
	char* cDirName = malloc(strlen(argv[1]) - 4);
	strncpy(cDirName, argv[1], strlen(argv[1]) - 4);
	strcat(cFile, cDirName);
	strcat(cFile, ".c");

	// create .h file name from input
	char* hFile = malloc(strlen(argv[2]) - 2);
	char* hDirName = malloc(strlen(argv[1]) - 4);
	strncpy(hDirName, argv[1], strlen(argv[1]) - 4);
	strcat(hFile, cDirName);
	strcat(hFile, ".h");

	// start .c file
	FILE *foutc = fopen(cFile, "w");

	// start .h file
	FILE *fouth = fopen(hFile, "w");
	fprintf(fouth, "#ifndef __%sH__\n", name);
	fprintf(fouth, "#define __%sH__\n\n", name);


	if (argc != 3) {
		printf("USAGE: wav2gba input.wav array_name (Input File must be 8-bit, MONO)\n\n");
		return EXIT_SUCCESS;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		return EXIT_FAILURE;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return EXIT_FAILURE;
	}

	/* mmap input file into memory */
	input_file_buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (input_file_buffer == MAP_FAILED) {
		close(fd);
		return EXIT_FAILURE;
	}

	if (st.st_size < WAVE_HEADER_LENGTH) {
		printf("ERROR: Input File is not a WAV file\n\n");
		return EXIT_FAILURE;
	}

	/* Check that the file is a valid 8-bit MONO WAV */
	memcpy(&wave_header, input_file_buffer, WAVE_HEADER_LENGTH);

	if (wave_header.channels != 1) {
		printf("ERROR: Input File is not MONO\n\n");
		return EXIT_FAILURE;
	}

	if (wave_header.bits_per_sample != 8) {
		printf("ERROR: Input File is not 8-bit\n\n");
		return EXIT_FAILURE;
	}

	// .h len macro and extern array
	fprintf(fouth, "#define %sLEN %llu\n", name, st.st_size - WAVE_HEADER_LENGTH);
	fprintf(fouth, "extern const signed char %s[%llu];\n\n", argv[2], st.st_size - WAVE_HEADER_LENGTH);

	// assign array in .c
	fprintf(foutc, "const signed char %s[%llu] = {", argv[2], st.st_size - WAVE_HEADER_LENGTH);
	for (size_t loop = 0; loop < st.st_size - WAVE_HEADER_LENGTH; loop++) {
		if (loop % 10 == 0) {
			fprintf(foutc, "\n\t");
		}
		
		signed char value = (signed char) input_file_buffer[WAVE_HEADER_LENGTH + loop] + 128;
		
		// probably unnecessary
		if (value < -128) {
			value = -128;
		} else if (value > 127) {
			value = 127;
		}

		if (loop == st.st_size - WAVE_HEADER_LENGTH - 1) {
			fprintf(foutc, "%d", ((signed char) value));
		} else {
			fprintf(foutc, "%d,", ((signed char) value));
		}
	}

	fprintf(foutc, "\n};\n");
	fprintf(fouth, "#endif");


	/* Terminate Program */
	munmap(input_file_buffer, st.st_size);
	close(fd);

	return EXIT_SUCCESS;
}
