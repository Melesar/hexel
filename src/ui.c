#include <ncurses.h>
#include "buffer.h"

#define BYTES_PER_ROW 16
#define BYTES_PER_OCTET 2

static buffer* buff;
static size_t win_size_rows, win_size_columns;

static void draw_byte(unsigned char byte, int is_selected) {
	int attributes = A_REVERSE;
	if (is_selected)
	{
		attron(attributes);
	}

	printw("%02x", byte);

	if (is_selected)
	{
		attroff(attributes);
	}
}

static void draw_text(unsigned char* line_start) {
	printw(" | ");
	for (size_t j = 0; j < BYTES_PER_ROW; ++j) {
		unsigned char byte = line_start[j];
		if (byte > 32 && byte < 127) {
			addch(byte);
		} else {
			addch('.');
		}
	}
			
}

static void draw_window() {
    unsigned char* screen_buffer;
    size_t starting_file_offset;
    int64_t buffer_length =
        buffer_get_screen_buffer(buff, &screen_buffer, &starting_file_offset);
    if (buffer_length < 0) {
        addstr("Failed to draw a window");
        refresh();
        return;
    }

    uint64_t cursor_row, cursor_column;
    buffer_get_cursor_pos(buff, &cursor_row, &cursor_column);
	uint64_t cursor_byte_position = cursor_row * BYTES_PER_ROW + cursor_column;

    for (size_t i = 0; i < buffer_length; ++i) {
        if (i != 0 && i % BYTES_PER_ROW == 0) {
			draw_text(screen_buffer + i - BYTES_PER_ROW);
            addch('\n');
        }

        if (i % BYTES_PER_ROW == 0) {
            printw("%08zx | ", starting_file_offset + i);
        }

        unsigned char byte = screen_buffer[i];
		draw_byte(byte, cursor_byte_position == i);

        if (i != 0 && (i - 1) % BYTES_PER_OCTET == 0) {
            addch(' ');
        }
    }
	
	draw_text(screen_buffer + (buffer_length - 1) / BYTES_PER_ROW * BYTES_PER_ROW);
    addch('\n');
    refresh();
}

int ui_init(buffer* buf) {
    initscr();
    raw();
    noecho();
    getmaxyx(stdscr, win_size_rows, win_size_columns);

    buff = buf;
    buffer_update_size(buff, win_size_rows, BYTES_PER_ROW);

    return 0;
}

void ui_start() {
    draw_window();

    while (1) {
        int c = getch();
        if (c == 'q') {
            break;
        }

		if (c == 'j') {
			buffer_move_cursor(buff, 1, 0);
		} else if (c == 'k') {
			buffer_move_cursor(buff, -1, 0);
		} else if (c == 'l') {
			buffer_move_cursor(buff, 0, 1);
		} else if (c == 'h') {
			buffer_move_cursor(buff, 0, -1);
		}

		clear();
		draw_window();
    }
}

void ui_destroy() {
    endwin();
}
