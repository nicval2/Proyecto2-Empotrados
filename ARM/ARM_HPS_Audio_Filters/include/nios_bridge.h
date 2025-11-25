// include/nios_bridge.h
#ifndef NIOS_BRIDGE_H
#define NIOS_BRIDGE_H

#include "audio_filter.h" // Necesita la definición de audio_buffer_t

// Placeholder: Dirección de memoria donde reside el búfer de comunicación NIOS/ARM
// Esto se define en Platform Designer y se obtiene del .sopcinfo (ej: HPS_TO_FPGA_BRIDGE)
#define NIOS_SHARED_MEMORY_ADDR ( (volatile uint32_t *) 0xC0000000 )

// Placeholder: Dirección de registro de control (banderas o semáforos)
// NIOS_STATUS: El NIOS escribe aquí para indicar que el búfer está libre.
#define NIOS_CONTROL_STATUS ( (volatile uint32_t *) 0xFF200000 )

void NIOS_BRIDGE_init();
void NIOS_BRIDGE_write_audio_chunk(const audio_buffer_t *buffer);

#endif // NIOS_BRIDGE_H
