/* vga.c - Optimizado sin librerías estándar */
#include "vga.h"

// Punteros directos
#define VGA_BUF_PTR  ((volatile unsigned short *)VGA_BASE)
#define VGA_CTRL_PTR ((volatile unsigned int *)VGA_CTRL_BASE)

/* Helper privado para longitud de string (reemplaza strlen de string.h) */
static int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

void vga_init(void) {
    VGA_CTRL_PTR[0] = 0;
    VGA_CTRL_PTR[1] = VGA_BASE;
    VGA_CTRL_PTR[0] = 1;
    vga_clear();
}

void vga_clear(void) {
    // Optimización: desenrollar bucle si se desea velocidad,
    // pero para tamaño binario el bucle simple es mejor.
    for (int i = 0; i < VGA_MEM_COLS * VGA_MEM_ROWS; i++)
        VGA_BUF_PTR[i] = (unsigned short)' ';
}

void vga_print(int row, int col, const char *msg) {
    if (row < 0 || row >= VGA_ROWS) return;
    if (col < 0 || col >= VGA_COLS) return;

    int pos = row * VGA_MEM_COLS + col;
    int i = 0;

    while (msg[i] != '\0') {
        if (col + i >= VGA_COLS) break;
        VGA_BUF_PTR[pos + i] = (unsigned short)msg[i];
        i++;
    }
}

void vga_print_center(const char *msg, int row) {
    if (row < 0 || row >= VGA_ROWS) return;

    int len = my_strlen(msg);

    // Quitar espacios finales (trim right)
    while (len > 0 && msg[len - 1] == ' ') len--;
    if (len <= 0) return;

    // Calcular columna inicial
    int col = (VGA_COLS - len) / 2;
    col -= VGA_X_SHIFT_CHARS;

    if (col < 0) col = 0;
    if (col > VGA_COLS - len) col = VGA_COLS - len;

    vga_print(row, col, msg);
}

void vga_clear_line(int row) {
    if (row < 0 || row >= VGA_ROWS) return;

    int pos = row * VGA_MEM_COLS;
    for (int c = 0; c < VGA_COLS; c++)
        VGA_BUF_PTR[pos + c] = (unsigned short)' ';
}
