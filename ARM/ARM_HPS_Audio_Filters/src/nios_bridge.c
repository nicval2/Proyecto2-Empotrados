// ----------------------------------------------------------------------
// src/nios_bridge.c

#include "../include/nios_bridge.h"
#include <string.h>

void NIOS_BRIDGE_init() {
    // Configura la dirección base y posiblemente resetea el registro de control
    *NIOS_CONTROL_STATUS = 0; // Establece el semáforo inicial a 0 (por ejemplo, buffer libre)
}

void NIOS_BRIDGE_write_audio_chunk(const audio_buffer_t *buffer) {

    // --- 1. Esperar el Semáforo ---
    // Espera activa (polling) hasta que el NIOS II señale que ha consumido el búfer anterior
    // Asumimos que NIOS_CONTROL_STATUS = 1 significa "NIOS está listo para recibir"
    while (*NIOS_CONTROL_STATUS != 1) {
        // En un sistema real con RTOS, usarías una interrupción o un semáforo de RTOS
    }

    // --- 2. Transferencia de Datos ---
    // Copia los datos del buffer local del ARM a la memoria compartida (FIFO/Bridge)
    // Usamos el tamaño en bytes (BUFFER_SIZE * 2 bytes por sample_t)
    memcpy((void *)NIOS_SHARED_MEMORY_ADDR,
           (const void *)buffer->samples,
           buffer->num_samples * sizeof(sample_t));

    // --- 3. Señalizar al NIOS ---
    // Una vez escrito, señaliza al NIOS II que hay datos disponibles (ej. establece el semáforo a 0)
    *NIOS_CONTROL_STATUS = 0;
}
