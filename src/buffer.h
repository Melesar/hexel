#ifndef BUFFER_H
#define BUFFER_H

#include "file.h"

#include <stddef.h>
#include <stdint.h>

#define ENDIAN_LE 0 
#define ENDIAN_BE 1

typedef struct buffer buffer;

buffer* buffer_init(file_stream* fs);

void buffer_update_size(buffer* buf, uint64_t num_rows, uint64_t bytes_per_row);

void buffer_move_cursor(buffer* buf, int64_t delta_row, int64_t delta_column); 
void buffer_get_cursor_pos(const buffer* buf, uint64_t* cursor_row, uint64_t* cursor_column);

int buffer_get_endiannes(const buffer* buf);
void buffer_set_endiannes(buffer* buf, int endiannes);

int64_t buffer_get_screen_buffer(const buffer* buf, unsigned char** buffer, size_t* starting_file_offset);
void* buffer_get_data_at_cursor(buffer* buf, size_t size);

void buffer_free(buffer* buf);

#endif
