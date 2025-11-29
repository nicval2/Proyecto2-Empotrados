#ifndef VGA_H
#define VGA_H

#include "system.h"
#include <stdint.h>

#define VGA_BASE      VGA_BUFFER_AVALON_CHAR_BUFFER_SLAVE_BASE
#define VGA_CTRL_BASE VGA_BUFFER_AVALON_CHAR_CONTROL_SLAVE_BASE

void vga_init(void);
void vga_clear(void);
void vga_print(int row, int col, const char *msg);
void vga_print_center(const char *msg, int row);

#endif
