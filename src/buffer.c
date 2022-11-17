#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "file.h"

#define BUFFER_SIZE 8

typedef struct buffer {
	uint64_t num_rows;
	uint64_t bytes_per_row;

	uint64_t cursor_row, cursor_column;

	int endiannes;

	unsigned char data_buffer[BUFFER_SIZE];
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


int buffer_get_endiannes(const buffer* buf) {
	return buf->endiannes;
}

void buffer_set_endiannes(buffer* buf, int endiannes) {
	buf->endiannes = endiannes > 0 ? ENDIAN_BE : ENDIAN_LE;
}

void* buffer_get_data_at_cursor(buffer* buf, size_t size) {
	void* file_data = file_get_data(buf->fs);
	memcpy(buf->data_buffer, file_data, size); //TODO make sure size is not bigger than the buffer size
	
	void* result = buf->data_buffer;
	if (buf->endiannes > 0) {
		for(size_t i = BUFFER_SIZE - 1; i >= BUFFER_SIZE - BUFFER_SIZE / 2; --i) {
			unsigned char tmp = buf->data_buffer[i];
			buf->data_buffer[i] = buf->data_buffer[BUFFER_SIZE - i - 1];
			buf->data_buffer[BUFFER_SIZE - i - 1] = tmp;
		}
		result += BUFFER_SIZE - size;
	}

	return result;
}

void buffer_set_cursor(buffer* buf, int64_t row, int64_t column);

void buffer_move_cursor(buffer* buf, int64_t delta_row, int64_t delta_column) {
	int64_t current_row = buf->cursor_row;
	int64_t current_column = buf->cursor_column;
	buffer_set_cursor(buf, current_row + delta_row, current_column + delta_column);
}

void buffer_set_cursor(buffer* buf, int64_t row, int64_t column) {
	if (column >= (int64_t)buf->bytes_per_row) {
		column = buf->bytes_per_row - 1;
	} else if (column < 0) {
		column = 0;
	}

	int64_t cursor_current_bytes = buf->cursor_row * buf->bytes_per_row + buf->cursor_column + 1;
	int64_t cursor_new_bytes = row * (int64_t) buf->bytes_per_row + column + 1;
	int64_t bytes_diff = cursor_new_bytes - cursor_current_bytes;

	if (row >= (int64_t)buf->num_rows) {
		buf->cursor_row = buf->num_rows - 1;
	} else if (row < 0){
		buf->cursor_row = 0;
	} else {
		buf->cursor_row = row;
	}

	buf->cursor_column = column;

	int64_t file_size = file_get_size(buf->fs);
	int64_t current_file_offset = file_get_offset(buf->fs);
	int64_t new_file_offset;
	
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
