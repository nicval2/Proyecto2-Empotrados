
#include "sys/alt_stdio.h"

#define SEGMENTS7_BASE 0x3000
#define TIMER_BASE 0x3020


void display_time_4seg(volatile int *hex_ptr,
                       int minutes, int seconds)
{
    // Tabla 7 segmentos
	int seg7_table[10] = {
	    0x40, 0x79, 0x24, 0x30, 0x19,
	    0x12, 0x02, 0x78, 0x00, 0x10
	};

    // Obtener dígitos
    int m1 = (minutes / 10) % 10;
    int m0 = minutes % 10;

    int s1 = (seconds / 10) % 10;
    int s0 = seconds % 10;

    // Convertir a 7 segmentos
    int hex3 = seg7_table[m1];
    int hex2 = seg7_table[m0];
    int hex1 = seg7_table[s1];
    int hex0 = seg7_table[s0];

    // Empaquetar
    *hex_ptr = (hex3 << 24) | (hex2 << 16) | (hex1 << 8) | hex0;
}

int song_timer() {
	volatile unsigned int * timer_status_ptr = (unsigned int *) TIMER_BASE;
	volatile unsigned int * timer_ctrl_ptr = timer_status_ptr + 1;	// Offset + 1
	volatile unsigned int * segments7_ptr = (unsigned int *) SEGMENTS7_BASE;

	alt_putstr("Hello from Nios II!\n");

	// Verificar estado del timer
	if (*timer_status_ptr != 0) {
	  alt_printf("ERROR: status is not 0 -> %x/n", *timer_status_ptr);
	  return 0;
	}

	// Iniciar el timer
	alt_putstr("Turning on timer\n");
	*timer_ctrl_ptr = 0x6;

	while(*timer_status_ptr != 0x2);
	alt_putstr("Timer is running\n");

	// Obtener tiempo transcurrido en milisegundos:
	int elapsed_ms = 0;
	int elapsed_s = 0;
	int elapsed_m = 0;

	while (1) {
	  if (*timer_status_ptr == 0x3) {
		  elapsed_ms++;
		  *timer_status_ptr = 0; 	// Reiniciar el timer

		  // Contar segundos y minutos
		  if (elapsed_ms == 1000) {
			  elapsed_s++;
			  elapsed_ms = 0;
		  }
		  if (elapsed_s == 60) {
			  elapsed_m++;
			  elapsed_s = 0;

		  }

		  // Obtener representacion 7 segmentos del tiempo
		  display_time_4seg(segments7_ptr, elapsed_m, elapsed_s);
	  }
	}
}


