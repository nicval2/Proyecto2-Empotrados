#include "vga.h"
#include <string.h>
#include <stdio.h>

void vga_init(void)
{
    volatile uint32_t *ctrl = (uint32_t*)VGA_CTRL_BASE;

    // Configura front buffer
    ctrl[0] = 0;           // disable
    ctrl[1] = VGA_BASE;    // frame buffer base
    ctrl[0] = 1;           // enable / swap

    // Debug
    printf("CTRL0 = 0x%08X\n", (unsigned)ctrl[0]);
    printf("CTRL1 = 0x%08X\n", (unsigned)ctrl[1]);

    vga_clear();
}

void vga_clear(void)
{
    volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;

    for (int i = 0; i < VGA_MEM_COLS * VGA_MEM_ROWS; i++)
        vga[i] = (uint16_t)' ';
}

void vga_print(int row, int col, const char *msg)
{
    if (row < 0 || row >= VGA_ROWS) return;
    if (col < 0 || col >= VGA_COLS) return;

    volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;

    // stride REAL de memoria = 128 columnas
    int pos = row * VGA_MEM_COLS + col;

    for (int i = 0; msg[i] != 0; i++)
    {
        if (col + i >= VGA_COLS)
            break;   // no salir del linea visible 80 columnas

        vga[pos + i] = (uint16_t)msg[i];
    }
}

void vga_print_center(const char *msg, int row)
{
    if (row < 0 || row >= VGA_ROWS) return;

    int len = strlen(msg);

    // quita espacios al final
    while (len > 0 && msg[len - 1] == ' ')
        len--;

    if (len <= 0) return;

    // centrado logico en las 80 columnas
    int col = (VGA_COLS - len) / 2;

    // corrige desplazamiento fisico hacia la izquierda
    col -= VGA_X_SHIFT_CHARS;

    if (col < 0) col = 0;
    if (col > VGA_COLS - len) col = VGA_COLS - len;

    vga_print(row, col, msg);
}

void vga_clear_line(int row)
{
    if (row < 0 || row >= VGA_ROWS)
        return;

    volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;

    int pos = row * VGA_MEM_COLS;   // stride real de 128 columnas

    for (int c = 0; c < VGA_COLS; c++)   // limpiar solo 80 visibles
        vga[pos + c] = (uint16_t)' ';
}
