// test_fifo2_listen.c - Test simple para recibir datos de NIOS
// Compilar: arm-linux-gnueabihf-gcc -o test_fifo2 test_fifo2_listen.c -O2

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define H2F_BRIDGE_BASE     0xC0000000

// FIFO2: NIOS -> HPS (verificado del QSYS)
#define FIFO2_OUT_OFFSET     0x8960   // HPS lee datos aquí
#define FIFO2_OUT_CSR_OFFSET 0x8980   // HPS lee estado aquí

#define CSR_FILL_LEVEL  0x00
#define CSR_STATUS      0x04

#define NIOS_READY_TOKEN 0xCAFEBABE

int main() {
    int fd_mem;
    void *h2f_map;
    volatile uint32_t *fifo2_out;
    volatile uint8_t *fifo2_out_csr;
    
    setbuf(stdout, NULL);
    
    printf("=== TEST FIFO2 LISTENER ===\n");
    printf("Direcciones:\n");
    printf("  FIFO2.out     = 0x%08X + 0x%04X = 0x%08X\n", 
           H2F_BRIDGE_BASE, FIFO2_OUT_OFFSET, H2F_BRIDGE_BASE + FIFO2_OUT_OFFSET);
    printf("  FIFO2.out_csr = 0x%08X + 0x%04X = 0x%08X\n",
           H2F_BRIDGE_BASE, FIFO2_OUT_CSR_OFFSET, H2F_BRIDGE_BASE + FIFO2_OUT_CSR_OFFSET);
    printf("\n");
    
    // Abrir /dev/mem
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        printf("Error: No se pudo abrir /dev/mem - %s\n", strerror(errno));
        printf("Ejecutar como root: sudo ./test_fifo2\n");
        return 1;
    }
    
    // Mapear bridge H2F
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    if(h2f_map == MAP_FAILED) {
        printf("Error: mmap falló - %s\n", strerror(errno));
        printf("¿Está la FPGA configurada con el .sof?\n");
        close(fd_mem);
        return 1;
    }
    
    printf("mmap exitoso en %p\n\n", h2f_map);
    
    // Configurar punteros
    fifo2_out = (volatile uint32_t *)((uint8_t*)h2f_map + FIFO2_OUT_OFFSET);
    fifo2_out_csr = (volatile uint8_t *)h2f_map + FIFO2_OUT_CSR_OFFSET;
    
    // Leer registros CSR para verificar acceso
    printf("Verificando acceso al hardware...\n");
    uint32_t fill = *(volatile uint32_t *)(fifo2_out_csr + CSR_FILL_LEVEL);
    uint32_t status = *(volatile uint32_t *)(fifo2_out_csr + CSR_STATUS);
    printf("  CSR Fill Level: %u\n", fill);
    printf("  CSR Status:     0x%08X\n", status);
    printf("\n");
    
    // Vaciar cualquier dato previo en el FIFO
    printf("Vaciando FIFO2...\n");
    int discarded = 0;
    while(fill > 0) {
        uint32_t data = *fifo2_out;
        printf("  Descartado: 0x%08X\n", data);
        discarded++;
        fill = *(volatile uint32_t *)(fifo2_out_csr + CSR_FILL_LEVEL);
    }
    printf("Descartados %d elementos\n\n", discarded);
    
    // Bucle principal de escucha
    printf("Esperando datos del NIOS...\n");
    printf("(Ejecuta el programa NIOS ahora)\n");
    printf("Presiona Ctrl+C para salir\n\n");
    
    int count = 0;
    int last_print = 0;
    
    while(1) {
        // Leer fill level
        fill = *(volatile uint32_t *)(fifo2_out_csr + CSR_FILL_LEVEL);
        
        if(fill > 0) {
            // Hay datos - leer
            uint32_t data = *fifo2_out;
            printf("[%d] Recibido: 0x%08X", count++, data);
            
            if(data == NIOS_READY_TOKEN) {
                printf(" <-- NIOS_READY_TOKEN!");
            }
            printf("\n");
            
            last_print = 0;
        }
        else {
            // Sin datos - mostrar estado periódicamente
            last_print++;
            if(last_print >= 500) {  // Cada ~5 segundos
                printf("... esperando (fill=%u, status=0x%X)\n", 
                       fill, 
                       *(volatile uint32_t *)(fifo2_out_csr + CSR_STATUS));
                last_print = 0;
            }
        }
        
        usleep(10000);  // 10ms
    }
    
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    return 0;
}
