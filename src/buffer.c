#include "buffer.h"
#include <stdlib.h>

typedef struct buffer {
	uint64_t num_rows;
	uint64_t bytes_per_row;

	uint64_t cursor_row, cursor_column;

	file_stream* fs;
} buffer;

buffer* buffer_init(file_stream* fs) {
	buffer* buf = calloc(1, sizeof(buffer));
	buf->fs = fs;
	return buf;
}

void buffer_update_size(buffer* buf, uint64_t num_rows, uint64_t bytes_per_row) {
	buf->num_rows = num_rows;
	buf->bytes_per_row = bytes_per_row;

	//TODO update cursor position
}

void buffer_set_cursor(buffer* buf, int64_t row, uint64_t column) {
	if (column >= buf->bytes_per_row) {
		column = buf->bytes_per_row - 1;
	}

	int64_t cursor_current_bytes = buf->cursor_row * buf->bytes_per_row + buf->cursor_column + 1;
	int64_t cursor_new_bytes = row * buf->bytes_per_row + column + 1;
	int64_t bytes_diff = cursor_new_bytes - cursor_current_bytes;

	if (row < 0) {
		buf->cursor_row = 0;
	} else if (row >= buf->num_rows) {
		buf->cursor_row = buf->num_rows - 1;
	} else {
		buf->cursor_row = row;
	}

	buf->cursor_column = column;

	uint64_t file_size = file_get_size(buf->fs);
	uint64_t current_file_offset = file_get_offset(buf->fs);
	uint64_t new_file_offset;
	if (bytes_diff < 0 && -bytes_diff > current_file_offset) {
		new_file_offset = column;
	} else if (bytes_diff > file_size - current_file_offset) {
		new_file_offset = file_size - buf->bytes_per_row + column;
	} else {
		new_file_offset = current_file_offset + bytes_diff;
	}

	file_set_offset(buf->fs, new_file_offset);
}

int64_t buffer_get_screen_buffer(const buffer* buf, unsigned char** buffer, size_t* starting_file_offset) {
	uint64_t file_offset = file_get_offset(buf->fs);
	uint64_t bytes_before = buf->cursor_column + buf->cursor_row * buf->bytes_per_row;
	uint64_t bytes_after = buf->bytes_per_row - buf->cursor_column + (buf->num_rows - buf->cursor_row - 1) * buf->bytes_per_row;

	*starting_file_offset = file_offset - bytes_before;
	return file_read_chunk(buf->fs, *starting_file_offset, bytes_after + bytes_before, buffer);
}

void buffer_get_cursor_pos(const buffer* buf, uint64_t* cursor_row, uint64_t* cursor_column) {
	*cursor_row = buf->cursor_row;
	*cursor_column = buf->cursor_column;
}

void buffer_free(buffer* buf) {
	free(buf);
}
