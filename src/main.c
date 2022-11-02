#include "buffer.h"
#include "file.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/uio.h>
#include <sys/stat.h>

void parse_cursor_position(int num_args, char** args, int64_t* row, uint64_t* column);

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

	uint64_t cursor_column;
	int64_t cursor_row; 
	parse_cursor_position(argc, argv, &cursor_row, &cursor_column);

	struct buffer* buffer = buffer_init(fs);

	if (ui_init(buffer) != 0) {
		fprintf(stderr, "Failed to initialize ncurses");
		return 1;
	}

	ui_start();
	ui_destroy();

	buffer_free(buffer);
	file_free(fs);

	return 0;
}

void parse_cursor_position(int num_args, char** args, int64_t* row, uint64_t* column) {
	for (int i = 1; i < num_args - 1; i += 2) {
		char* flag = args[i];
		if (strncmp(flag, "-c", 2) == 0 && i + 1 < num_args - 1) {
			char* cursor_arg = args[i + 1];
			char* cursor_str = strtok(cursor_arg, ",");
			char* invalid;
			if (cursor_str == NULL) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			*row  = strtoul(cursor_str, &invalid, 10);
			if (invalid == cursor_str) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			cursor_str = strtok(NULL, ",");
			if (cursor_str == NULL) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

			*column = strtoull(cursor_str, &invalid, 10);
			if (invalid == cursor_str) {
				fprintf(stderr, "-c flag expects two integers separated with comma\n");
				continue;
			}

		}
	}
}

