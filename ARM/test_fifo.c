#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// ==========================================================
// --- PARÁMETROS CRÍTICOS DEL HARDWARE (ACTUALIZADO) ---
// ==========================================================
// Dirección Base del bus AXI HPS-a-FPGA (Valor estándar)
#define HPS_FPGA_BASE_ADDR     0xC0000000

// Offsets del FIFO (Tus nuevos valores de Platform Designer)
#define FIFO_DATA_OFFSET_RAW   0x8860
#define FIFO_CSR_OFFSET_RAW    0x8900

// --- PARÁMETROS DE MAPEO DE MEMORIA (Kernel) ---
#define PAGE_SIZE              0x1000 // Tamaño de página típico (4 KB)
#define PAGE_MASK              (~(PAGE_SIZE - 1))

// 1. Dirección Base de la PÁGINA ALINEADA:
// 0xC0000000 + 0x8000 = 0xC0008000
#define ALIGNED_ADDR_PHY       (HPS_FPGA_BASE_ADDR + (FIFO_DATA_OFFSET_RAW & PAGE_MASK))
#define MAP_SIZE               PAGE_SIZE

// 2. Calcular el Desplazamiento (OFFSET) DENTRO de la página mapeada:
// 0x8860 - 0x8000 = 0x860
#define DATA_PTR_OFFSET        (FIFO_DATA_OFFSET_RAW - (FIFO_DATA_OFFSET_RAW & PAGE_MASK)) 

// 0x8900 - 0x8000 = 0x900
#define CSR_PTR_OFFSET         (FIFO_CSR_OFFSET_RAW - (FIFO_CSR_OFFSET_RAW & PAGE_MASK)) 

#define TEST_VALUE 0xDEADBEEF 

int main() {
    int fd;
    volatile uint8_t *virtual_base = NULL;
    volatile uint32_t *fifo_data_ptr = NULL;
    volatile uint32_t *fifo_csr_ptr = NULL;
    uint32_t read_status = 0;

    printf("--- Test de Conexión HPS-a-FPGA con FIFO (Nuevas Direcciones) ---\n");
    printf("Dirección ALINEADA a mapear: 0x%08X\n", ALIGNED_ADDR_PHY);
    
    // 1. Abrir /dev/mem
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("Error abriendo /dev/mem");
        return 1;
    }

    // 2. Mapear la PÁGINA COMPLETA alineada (0xC0008000)
    virtual_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ALIGNED_ADDR_PHY);
    
    if (virtual_base == MAP_FAILED) {
        perror("\n!!! ERROR CRÍTICO en mmap !!!");
        close(fd);
        return 1;
    }
    printf("Página mapeada correctamente. Base virtual: %p\n", virtual_base);

    // 3. Crear punteros basados en el desplazamiento DENTRO de la página mapeada
    fifo_data_ptr = (volatile uint32_t *)(virtual_base + DATA_PTR_OFFSET);
    fifo_csr_ptr = (volatile uint32_t *)(virtual_base + CSR_PTR_OFFSET);
    
    // Debug para verificar que los punteros se calcularon correctamente
    printf("FIFO Data (Virtual) esperado: %p\n", fifo_data_ptr);
    printf("FIFO CSR (Virtual) esperado: %p\n", fifo_csr_ptr);

    // 4. Intentar Escribir en el FIFO Data
    printf("\nIntentando escribir 0x%08X en FIFO Data (offset +0x%X)... ", TEST_VALUE, DATA_PTR_OFFSET);
    *fifo_data_ptr = TEST_VALUE;
    printf("Escritura completada.\n");

    // 5. Intentar Leer el Registro de Control
    read_status = *fifo_csr_ptr;
    printf("Lectura de FIFO CSR (offset +0x%X) = 0x%08X\n", CSR_PTR_OFFSET, read_status);

    printf("\n--- Prueba Finalizada. ---\n");

    // Limpieza
    munmap((void*)virtual_base, MAP_SIZE);
    close(fd);
    return 0;
}