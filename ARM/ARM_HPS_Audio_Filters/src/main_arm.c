// src/main_arm.c

// *************** INCLUDES ***************
// Incluye todos los headers necesarios para orquestar los módulos.
#include "../include/audio_filter.h"
#include "../include/sd_manager.h"
#include "../include/nios_bridge.h"
#include <stdint.h>
// Nota: La inclusión de stdint.h aquí es vital, aunque Eclipse lo marque como error.

// *************** PROTOTIPOS DE FUNCIONES AUXILIARES ***************
void HPS_iniciar_perifericos();
filtro_t HPS_read_filter_switch();
void HPS_mostrar_error(uint8_t error_code);

// *************** DEFINICIONES ***************
// Los buffers globales que se usarán para transferir y procesar los datos.
// Esto simula memoria RAM.
audio_buffer_t input_buffer_A;
audio_buffer_t output_buffer_B;
audio_buffer_t processed_buffer_C;

// Variable de estado global (simula la lectura de switches de la placa)
filtro_t current_filter_setting = FILTRO_NINGUNO;


// *************** FUNCIÓN PRINCIPAL ***************
int main()
{
    // --- 1. Inicialización del Sistema ---

    // Inicialización del HPS (Bare Metal): Registros, relojes, etc.
    // En un sistema real, el BSP generado por EDS manejaría esto.
    HPS_iniciar_perifericos();

    // Inicialización del gestor de la Tarjeta SD (necesita el SO Mínimo/FATFS)
    if (SD_MANAGER_init("song.wav") != 0) {
        // En caso de fallo (ej. SD no encontrada), entra en bucle de error.
        HPS_mostrar_error(0xF0); // Muestra código F0 en display de 7 seg.
        while(1);
    }

    // Inicialización del puente de comunicación con el NIOS II
    NIOS_BRIDGE_init();

    // --- 2. Bucle de Procesamiento de Audio (Bucle Infinito) ---
    while (1)
    {
        // A. Lectura de Audio (SD -> Buffer A)
        // La función de lectura llenará el buffer A con BUFFER_SIZE muestras.
        int bytes_read = SD_MANAGER_read_audio_chunk(&input_buffer_A);

        // Si no se lee nada, significa fin del archivo.
        if (bytes_read <= 0) {
            // Reiniciar la canción o parar la reproducción
            SD_MANAGER_rewind();
            continue;
        }

        // B. Lectura del estado del filtro (simulación de GPIO)
        current_filter_setting = HPS_read_filter_switch();

        // C. Procesamiento DSP (Buffer A -> Buffer C)
        AUDIO_FILTRO_procesar_buffer(
            current_filter_setting,
            &input_buffer_A,
            &processed_buffer_C
        );

        // D. Escritura de Audio (Buffer C -> NIOS Shared Memory)
        // La función espera la señal de "listo" del NIOS antes de escribir.
        NIOS_BRIDGE_write_audio_chunk(&processed_buffer_C);

        // La iteración se repite
    }

    return 0; // El código Bare Metal nunca debería llegar aquí
}


// *************** FUNCIONES PLACEHOLDER NECESARIAS ***************

// Placeholder: Simula la inicialización de los registros internos del HPS.
void HPS_iniciar_perifericos() {
    // Aquí iría el código de inicialización del HPS
}

// Placeholder: Lee el estado de los switches para determinar el filtro.
filtro_t HPS_read_filter_switch() {
    // Debería leer el registro de GPIO del HPS conectado a los switches de la placa.
    // Por ahora, asumimos FILTRO_GRAVES para la prueba.
    return FILTRO_GRAVES;
}

// Placeholder: Función de error (muestra el código en los displays de 7 segmentos)
void HPS_mostrar_error(uint8_t error_code) {
    // Aquí iría el código para escribir el error_code en el registro de los displays.
}
