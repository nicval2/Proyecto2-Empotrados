#include "vga.h"
#include "system.h"
#include <string.h>

/* ============================================================
   INICIALIZACIÓN DEL CHARACTER BUFFER CON DMA
   ============================================================ */
void vga_init(void)
{
    volatile uint32_t *ctrl = (uint32_t*)VGA_CTRL_BASE;

    printf("CTRL0 = %08X\n", ctrl[0]);
    printf("CTRL1 = %08X\n", ctrl[1]);


    ctrl[1] = VGA_BASE;   // Dirección del front buffer
    ctrl[0] = 1;          // Swap buffer

    vga_clear();
}

/* ============================================================
   LIMPIAR PANTALLA COMPLETA
   ============================================================ */
void vga_clear(void)
{
    volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;

    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = (uint16_t)' ';
}

/* ============================================================
   ESCRIBIR TEXTO EN FILA/COLUMNA
   ============================================================ */
void vga_print(int row, int col, const char *msg)
{
    if (row < 0 || row >= VGA_ROWS) return;

    volatile uint16_t *vga = (volatile uint16_t *)VGA_BASE;
    int pos = row * VGA_COLS + col;

    for (int i = 0; msg[i] != 0; i++)
    {
        if (col + i >= VGA_COLS)
            break;

        vga[pos + i] = (uint16_t)msg[i];
    }
}

/* ============================================================
   IMPRIMIR TEXTO CENTRADO HORIZONTALMENTE
   ============================================================ */
void vga_print_center(const char *msg, int row)
{
    int len = strlen(msg);
    if (len > VGA_COLS) len = VGA_COLS;

    int col = (VGA_COLS - len) / 2;
    vga_print(row, col, msg);
}
