#include <ncurses.h>
#include <string.h>

#include "buffer.h"

#define BYTES_PER_ROW 16
#define BYTES_PER_OCTET 2
#define STATUS_MAX_LEN 52

#define ESC_KEY 27
#define BACKSPACE_KEY 127
#define CTRL_D 4
#define CTRL_U 21

typedef enum {
	Normal,
	Insert,
	Command
} mode;

struct normal_mode {
	char status_text[STATUS_MAX_LEN]; 
} nm;

struct command_mode {
	size_t cmd_len;
	char cmd_text[STATUS_MAX_LEN];
} cm;

static buffer* buff;
static size_t win_size_rows, win_size_columns;
static mode current_mode;

static int is_printable(int c) {
	return c > 32 && c < 127;
}

static void draw_byte(unsigned char byte, int is_selected) {
    int attributes = A_REVERSE;
    if (is_selected) {
        attron(attributes);
    }

    printw("%02x", byte);

    if (is_selected) {
        attroff(attributes);
    }
}

static void draw_text(unsigned char* line_start) {
    printw(" | ");
    for (size_t j = 0; j < BYTES_PER_ROW; ++j) {
        unsigned char byte = line_start[j];
        if (is_printable(byte)) {
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

    draw_text(screen_buffer +
              (buffer_length - 1) / BYTES_PER_ROW * BYTES_PER_ROW);
    addch('\n');

	switch(current_mode) {
		case Normal:
			if(strlen(nm.status_text) > 0) {
				mvaddstr(win_size_rows, 1, nm.status_text);
			}
			break;
		case Command:
			mvprintw(win_size_rows - 1, 0, ":%s", cm.cmd_text);
			break;
		default:
			break;
	}

	char* end = buffer_get_endiannes(buff) == ENDIAN_BE ? "BE" : "LE";
	mvaddstr(win_size_rows - 2, STATUS_MAX_LEN + 2, end);

	int* data_ptr = buffer_get_data_at_cursor(buff, 4);
	mvprintw(0, STATUS_MAX_LEN + BYTES_PER_ROW + 10, "int32: %d", *data_ptr);

    refresh();
}

int ui_init(buffer* buf) {
    initscr();
    raw();
    noecho();
    getmaxyx(stdscr, win_size_rows, win_size_columns);
	curs_set(0);

	memset(&nm, 0, sizeof nm);
	memset(&cm, 0, sizeof cm);
 
    buff = buf;
    buffer_update_size(buff, win_size_rows - 2, BYTES_PER_ROW);

    return 0;
}

static int input_normal_mode(int c) {
	if (c == 'q') {
		return 1;
	}

	if (c == ':') {
		current_mode = Command;
		return 0;
	}

	if (c == 'j') {
		buffer_move_cursor(buff, 1, 0);
	} else if (c == 'k') {
		buffer_move_cursor(buff, -1, 0);
	} else if (c == 'l') {
		buffer_move_cursor(buff, 0, 1);
	} else if (c == 'h') {
		buffer_move_cursor(buff, 0, -1);
	} else if (c == CTRL_D) {
		buffer_move_cursor(buff, win_size_rows / 2, 0);
	} else if (c == CTRL_U) {
		buffer_move_cursor(buff, -win_size_rows / 2, 0);
	}
	return 0;
}


static void clear_cmd_mode() {
	memset(&cm, 0, sizeof cm);
	current_mode = Normal;
}

static int input_command_mode(int c) {
	if (c == ESC_KEY) {
		clear_cmd_mode();
		return 0;
	}

	if (is_printable(c) && cm.cmd_len < STATUS_MAX_LEN - 1) {
		cm.cmd_text[cm.cmd_len++] = c;
		cm.cmd_text[cm.cmd_len] = '\0';
	} else if (c == BACKSPACE_KEY && cm.cmd_len > 0) {
		cm.cmd_text[--cm.cmd_len] = '\0';
	}


	return 0;
}

static int input_insert_mode(int c) {
	return 0;
}

void ui_start() {
    draw_window();

    while (1) {
		int c = getch();

		int should_quit;
		switch(current_mode) {
			case Normal:
				should_quit = input_normal_mode(c);
				break;
			case Insert:
				should_quit = input_insert_mode(c);
				break;
			case Command:
				should_quit = input_command_mode(c);
				break;
		}

		if(should_quit) {
			break;
		}

        clear();
        draw_window();
    }
}

void ui_destroy() {
    endwin();
}
