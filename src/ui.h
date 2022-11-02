#ifndef UI_H
#define UI_H

struct buffer;

int ui_init(buffer* buf);
void ui_start();
void ui_destroy();

#endif
