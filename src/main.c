#include <stdio.h>
#include "file.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/stat.h>

#define LINE_LENGTH 10
#define NUM_LINES 10
#define BYTES_PER_SCREEN (LINE_LENGTH * NUM_LINES)

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "File name is required\n");
		return 1;
	}

	struct file_stream* fs = file_init(argv[1]);
	if (fs == NULL) {
		fprintf(stderr, "Failed to initialize the file stream\n");
		return 1;
	}

	printf("File is %lu bytes in size\n", file_get_size(fs));

	size_t offset;
	unsigned char* buffer;
	while(1) {
		printf("Enter offset and length: ");
		scanf("%lu", &offset);

		file_set_offset(fs, offset);
		int bytes_read = file_read_chunk(fs, offset, BYTES_PER_SCREEN, &buffer);
		if (bytes_read < 0) {
			continue;
		}

		for (size_t i = 0; i < bytes_read; ++i) {
			if (i > 0 && i % LINE_LENGTH == 0) {
				printf("\n");
			}

			unsigned char byte = buffer[i];
			printf("%02x ", byte);
		}

		printf("\n");
	}

	file_free(fs);

	return 0;
}
