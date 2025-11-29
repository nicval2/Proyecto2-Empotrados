#include "vga.h"
#include <string.h>
#include <stdio.h>

// 80x? visibles (lo que tengas en el IP)
#define VGA_COLS       80
#define VGA_ROWS       30      // o 60 si estás usando 80x60

// Tamaño real de la memoria: 8192 bytes => 4096 halfwords => 128x32
#define VGA_MEM_COLS   128
#define VGA_MEM_ROWS   32

// Ajuste horizontal en caracteres (positivo = mueve todo a la IZQUIERDA)
#define VGA_X_SHIFT_CHARS   20   // PRUEBA con 4, 6, 8 hasta que se vea centrado

void vga_init(void)
{
    volatile uint32_t *ctrl = (uint32_t*)VGA_CTRL_BASE;

    // Configura front buffer
    ctrl[0] = 0;           // disable
    ctrl[1] = VGA_BASE;    // frame buffer base
    // Muchos cores ignoran [2] y [3], pero los dejamos en 0 por si acaso
    // ctrl[2] = 0;
    // ctrl[3] = 0;
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
            break;   // no salir del área visible 80 columnas

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

    // centrado lógico en las 80 columnas
    int col = (VGA_COLS - len) / 2;

    // corrige desplazamiento físico hacia la izquierda
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

