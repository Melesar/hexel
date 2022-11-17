#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdint.h>

typedef struct file_stream file_stream;

typedef struct {
    unsigned char* ptr;
    size_t length;
} file_chunk;

file_stream* file_init(char* file_name);

size_t file_get_size(const file_stream* fs);
size_t file_get_offset(const file_stream* fs);

void file_set_offset(file_stream* fs, size_t offset);

void* file_get_data(const file_stream* fs);
int64_t file_read_chunk(const file_stream* fs, size_t offset, size_t length, unsigned char** buffer);

void file_free(struct file_stream* fs);

#endif
