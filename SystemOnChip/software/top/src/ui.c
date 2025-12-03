#include "ui.h"
#include "vga.h"
#include "filter.h"
#include <stdio.h>
#include <string.h>

/* Inicializacion de la interfaz */
void ui_init(void)
{
    vga_init();
}

/* Pantalla de espera */
void ui_show_waiting(void)
{
    vga_clear();

    vga_print_center("=== REPRODUCTOR FPGA ===", 3);
    vga_print_center("Esperando HPS...", 7);

    vga_print_center("CONTROLES:", 12);
    vga_print_center("KEY3: Play/Pause", 14);
    vga_print_center("KEY2: Siguiente", 15);
    vga_print_center("KEY1: Anterior", 16);

    vga_print_center("ECUALIZADOR:", 20);
    vga_print_center("SW0:Bass+ SW1:Bass- SW2:Treble+", 22);
    vga_print_center("SW3:Treble- SW4:Vocal SW5:Rock", 23);
    vga_print_center("SW6:Pop SW7:Jazz", 24);
}

/* Pantalla de reproduccion */
void ui_show_now_playing(void)
{
    extern char title_str[];
    extern char artist_str[];
    extern char album_str[];

    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 2);

    vga_print_center(title_str, 5);
    vga_print_center(artist_str, 7);
    vga_print_center(album_str, 9);

    vga_print_center("TIME:", 13);
    ui_update_timer(0);

    ui_update_filter();

    vga_print_center("KEY3:Prev KEY2:Play/Pause KEY1:Next", 25);
    vga_print_center("SW0-7: Ecualizador", 27);
}

/* Actualizacion de tiempo */
void ui_update_timer(uint32_t seconds)
{
    char buf[16];
    sprintf(buf, "%02lu:%02lu", seconds / 60, seconds % 60);
    vga_print_center(buf, 15);
}

/* Mostrar el filtro actual */
void ui_update_filter(void)
{
    vga_clear_line(17);

    char buf[32];
    sprintf(buf, "EQ: %s", filter_get_name());
    vga_print_center(buf, 17);
}

/* Mensaje de transicion entre canciones */
void ui_transition(void)
{
    vga_clear_line(20);
    vga_print_center(">> Cambiando pista... <<", 20);
}
