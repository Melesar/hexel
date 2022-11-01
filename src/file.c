#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "file.h"

#define CHUNK_SIZE 4096

typedef struct {
	size_t offset;
	size_t length;
} chunk_t;

typedef struct file_stream {
	int fd;
	size_t file_size;
	size_t offset;

	chunk_t chunks[3];

	unsigned char* buffer;
} file_stream;

static size_t min(size_t a, size_t b) {
	return a < b ? a : b;
}

static void load_new_chunks(file_stream* fs, size_t offset) {
	size_t current_chunk_offset = offset / CHUNK_SIZE * CHUNK_SIZE;
	size_t bytes_left = fs->file_size - current_chunk_offset;
	size_t current_chunk_length = min(bytes_left, CHUNK_SIZE);
	
	fs->chunks[1].offset = current_chunk_offset;
	fs->chunks[1].length = current_chunk_length;

	size_t next_chunk_offset = current_chunk_offset + current_chunk_length;
	if (next_chunk_offset < fs->file_size) {
		fs->chunks[2].offset = next_chunk_offset;
		fs->chunks[2].length = min(fs->file_size - next_chunk_offset, CHUNK_SIZE);
	} else {
		fs->chunks[2].offset = fs->chunks[2].length = 0;
	}

	fs->chunks[0].offset = current_chunk_offset > CHUNK_SIZE ? current_chunk_offset - CHUNK_SIZE : 0;
	fs->chunks[0].length = current_chunk_offset - fs->chunks[0].offset;

	struct iovec iovs[3];
	size_t iov_index = 0;
	int64_t file_offset = -1;
	for (size_t i = 0; i < 3; ++i) {
		chunk_t chunk = fs->chunks[i];
		if (chunk.offset == 0 && chunk.length == 0) {
			continue;
		}

		iovs[iov_index].iov_base = fs->buffer + i * CHUNK_SIZE;
		iovs[iov_index].iov_len = chunk.length;
		iov_index += 1;

		if (file_offset < 0) {
			file_offset = chunk.offset;
		}
	}

	if (preadv(fs->fd, iovs, iov_index, file_offset) < 0) {
		perror("preadv:load_new_chunks");
		return;
	}
}

/*
 *	Unloads the chunk given by chunk_index and loads a new one on the other side
 */
static void swap_chunk(file_stream* fs, size_t chunk_index) {
	if (chunk_index >= 3 || chunk_index == 1) {
		return;
	}
	
	int direction = 1 - (int)chunk_index;
	for (size_t i = chunk_index + direction; i < 3; i += direction) {
		unsigned char* src = fs->buffer + i * CHUNK_SIZE;
		unsigned char* dest = fs->buffer + (i - direction) * CHUNK_SIZE;
		size_t n = fs->chunks[i].length;

		memcpy(dest, src, n);
		fs->chunks[i - direction] = fs->chunks[i];
	}

	size_t new_chunk_index = 1 + direction;
	chunk_t* new_chunk = &fs->chunks[new_chunk_index];
	const chunk_t* middle_chunk = &fs->chunks[1];
	if (middle_chunk->length < CHUNK_SIZE) {
		new_chunk->offset = 0;
		new_chunk->length = 0;
		return;
	}

	size_t new_chunk_offset, new_chunk_length;
	switch(new_chunk_index) {
		case 0:
			new_chunk_offset = middle_chunk->offset - CHUNK_SIZE;
			new_chunk_length = CHUNK_SIZE;
			break;
		case 2:
			if (middle_chunk->offset + 2 * CHUNK_SIZE > fs->file_size)  {
				new_chunk_length = fs->file_size - middle_chunk->offset - CHUNK_SIZE;
			} else {
				new_chunk_length = CHUNK_SIZE;
			}
			new_chunk_offset = middle_chunk->offset + CHUNK_SIZE;
			break;
		default:
			new_chunk_offset = 0;
			new_chunk_length = 0;
			break;
	}

	new_chunk->offset = new_chunk_offset;
	new_chunk->length = new_chunk_length;

	if (pread(fs->fd, fs->buffer + new_chunk_index * CHUNK_SIZE, new_chunk->length, new_chunk->offset) < 0) {
		perror("pread");
	}
}

file_stream* file_init(char* file_name) {
	file_stream* fs = calloc(1, sizeof(file_stream));
	fs->fd = open(file_name, O_RDONLY);
	if (fs->fd < 0) {
		perror("open");
		free(fs);
		return NULL;
	}

	struct stat stat;
	if (fstat(fs->fd, &stat) < 0) {
		perror("fstat");
		close(fs->fd);
		free(fs);
		return NULL;
	}

	fs->file_size = stat.st_size;
	fs->offset = 0;

	fs->buffer = calloc(3, CHUNK_SIZE);
	if (fs->buffer == NULL) {
		perror("buffer calloc");
		close(fs->fd);
		free(fs);
		return NULL;
	}

	struct iovec iovs[2];
	size_t iovs_count = fs->file_size > CHUNK_SIZE ? 2 : 1;
	for (size_t i = 0; i < iovs_count; ++i) {
		size_t offset = i * CHUNK_SIZE;
		size_t bytes_left = fs->file_size - offset;
		size_t buffer_size = bytes_left > CHUNK_SIZE ? CHUNK_SIZE : bytes_left;

		iovs[i].iov_base = fs->buffer + (i + 1) * CHUNK_SIZE;
		iovs[i].iov_len = buffer_size;
	}
	
	if (readv(fs->fd, iovs, iovs_count) < 0) {
		perror("readv");
		file_free(fs);
		return NULL;
	}

	fs->chunks[0].offset = 0;
	fs->chunks[0].length = 0;
	for (size_t i = 1; i < 3; ++i) {
		fs->chunks[i].offset = iovs_count >= i ? (i - 1) * CHUNK_SIZE : fs->file_size;
		fs->chunks[i].length = iovs_count >= i ? iovs[i - 1].iov_len : 0;
	}

	return fs;
}

void file_set_offset(file_stream* fs, size_t offset) {
	if (offset > fs->file_size) {
		offset = fs->file_size;
	}

	chunk_t* previous_chunk = &fs->chunks[0];
	chunk_t* current_chunk = &fs->chunks[1];
	chunk_t* next_chunk = &fs->chunks[2];
	if (offset < current_chunk->offset) { //Before the current chunk
		if (offset < previous_chunk->offset || previous_chunk->length < CHUNK_SIZE) {
			load_new_chunks(fs, offset);
			return;
		}

		swap_chunk(fs, 2);
	} else if (offset > current_chunk->offset + current_chunk->length) { //After the current chunk
		if (offset > next_chunk->offset + next_chunk->length || next_chunk->length < CHUNK_SIZE) {
			load_new_chunks(fs, offset);
			return;
		}
	
		swap_chunk(fs, 0);
	}

	fs->offset = offset;
}

int64_t file_read_chunk(const file_stream *fs, size_t offset, size_t length, unsigned char** buffer) {
	if (offset >= fs->file_size) {
		return -1;
	}

	for(size_t i = 0; i < 3; ++i) {
		chunk_t chunk = fs->chunks[i];
		if (offset >= chunk.offset && offset < chunk.offset + chunk.length) {
			*buffer = fs->buffer + i * CHUNK_SIZE + (offset - chunk.offset);
			length = fs->file_size - offset <= length ? fs->file_size - offset : length;
			return length;
		}
	}

	return -1;
}

size_t file_get_size(const file_stream* fs) {
	return fs->file_size;
}

size_t file_get_offset(const file_stream* fs) {
	return fs->offset;
}

void file_free(struct file_stream* fs) {
	close(fs->fd);
	free(fs->buffer);
	free(fs);
}
