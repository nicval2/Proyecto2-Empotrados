#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define LW_BRIDGE_BASE  0xFF200000
#define LW_BRIDGE_SPAN  0x00200000

#define FIFO_IN_CSR_OFFSET  0x3100
#define FIFO_IN_DATA_OFFSET 0x3060

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    void *lw_base = mmap(NULL, LW_BRIDGE_SPAN,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fd, LW_BRIDGE_BASE);

    if (lw_base == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    volatile uint32_t *fifo_csr =
        (uint32_t *)((char *)lw_base + FIFO_IN_CSR_OFFSET);

    printf("FIFO CSR RAW: 0x%08X\n", *fifo_csr);

    munmap(lw_base, LW_BRIDGE_SPAN);
    close(fd);

    return 0;
}
