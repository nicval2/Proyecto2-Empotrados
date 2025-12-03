#ifndef VGA_H
#define VGA_H

#include "system.h"
#include <stdint.h>

/* --- Bases del buffer y control VGA --- */
#define VGA_BASE        VGA_CONFIG_AVALON_CHAR_BUFFER_SLAVE_BASE
#define VGA_CTRL_BASE   VGA_CONFIG_AVALON_CHAR_CONTROL_SLAVE_BASE

/* --- Dimensiones visibles del display --- */
#define VGA_COLS        80
#define VGA_ROWS        30

/* --- Dimensiones reales de memoria VGA --- */
#define VGA_MEM_COLS    128
#define VGA_MEM_ROWS    32

/* --- Ajuste horizontal global en caracteres --- */
/* Valor positivo mueve el texto hacia la izquierda */
#define VGA_X_SHIFT_CHARS   20

/* --- Prototipos --- */
void vga_init(void);
void vga_clear(void);
void vga_print(int row, int col, const char *msg);
void vga_print_center(const char *msg, int row);
void vga_clear_line(int row);

#endif
