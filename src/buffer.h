#ifndef BUFFER_H
#define BUFFER_H

#include "file.h"

#include <stddef.h>
#include <stdint.h>

typedef struct buffer buffer;

buffer* buffer_init(file_stream* fs);

void buffer_update_size(buffer* buf, uint64_t num_rows, uint64_t bytes_per_row);
void buffer_set_cursor(buffer* buf, int64_t row, uint64_t column);

int64_t buffer_get_screen_buffer(const buffer* buf, unsigned char** buffer);
void buffer_get_cursor_pos(const buffer* buf, uint64_t* cursor_row, uint64_t* cursor_column);

void buffer_free(buffer* buf);

#endif
