#include "buffer.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/uio.h>
#include <sys/stat.h>

#define LINE_LENGTH 10
#define NUM_LINES 10
#define BYTES_PER_SCREEN (LINE_LENGTH * NUM_LINES)

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: hexel -c <cursor_x,cursor_y> file_name");
		return 1;
	}

	char* file_name = argv[argc - 1];
	struct file_stream* fs = file_init(file_name);
	if (fs == NULL) {
		return 1;
	}

	uint64_t cursor_column = 0;
	int64_t cursor_row = 0; 
	for (int i = 1; i < argc - 1; i += 2) {
		char* flag = argv[i];
		if (strncmp(flag, "-c", 2) == 0 && i + 1 < argc - 1) {
			char* cursor_arg = argv[i + 1];
			char* cursor_str = strtok(cursor_arg, ",");
			char* invalid;
			if (cursor_str == NULL) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			cursor_row  = strtoul(cursor_str, &invalid, 10);
			if (invalid == cursor_str) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			cursor_str = strtok(NULL, ",");
			if (cursor_str == NULL) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			cursor_column  = strtoull(cursor_str, &invalid, 10);
			if (invalid == cursor_str) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

		}
	}

	struct buffer* buffer = buffer_init(fs);

	buffer_update_size(buffer, NUM_LINES, LINE_LENGTH);
	buffer_set_cursor(buffer, cursor_row, cursor_column);

	uint64_t row, column;
	buffer_get_cursor_pos(buffer, &row, &column);
	size_t file_offset = file_get_offset(fs);

	printf("File offset: %lu\n", file_offset);
	printf("Cursor: row %lu, column %lu\n", row, column);

	unsigned char* screen_buffer;
	int64_t buffer_len = buffer_get_screen_buffer(buffer, &screen_buffer);
	if (buffer_len < 0) {
		return 1;
	}

	for (size_t i = 0; i < buffer_len; ++i) {
		if (i != 0 && i % LINE_LENGTH == 0) {
			printf("\n");
		}

		unsigned char byte = screen_buffer[i];
		printf("%02x ", byte);
	}

	printf("\n");

	buffer_free(buffer);
	file_free(fs);

	return 0;
}
