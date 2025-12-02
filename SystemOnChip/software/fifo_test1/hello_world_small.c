/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include "system.h"
#include "altera_avalon_fifo_util.h"
#include "altera_avalon_fifo_regs.h"

int main() {

	alt_printf("FIFO test start.\n");

	// -------------------- PARTE IN (DEL ARM) --------------------

	// Para verificar si esta vacio
	if (altera_avalon_fifo_read_status(FIFO_IN_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK)) {
		alt_printf("FIFO vacio\n");
		// Otras posibilidades; full, empty, overflow, all
	}

    // --------- ESCRIBIR x2 -----------
	unsigned int value = 12344321;
	alt_printf("Escribiendo %x\n", value);
	altera_avalon_fifo_write_fifo(FIFO_IN_BASE, FIFO_IN_CSR_BASE, value);

	value = 3333333;
	alt_printf("Escribiendo %x\n", value);
	altera_avalon_fifo_write_fifo(FIFO_IN_BASE, FIFO_IN_CSR_BASE, value);

	// -------------------- PARTE OUT (DEL NIOS) --------------------

	// Contar la cantidad de datos que tiene el FIFO
	unsigned int n = altera_avalon_fifo_read_level(FIFO_OUT_CSR_BASE);
	alt_printf("Cantidad de datos: %x\n", n);

	// Escribir en LEDS
	volatile unsigned int * leds_ptr = (unsigned int *) REG_7_SEGMENTS_BASE;
	*leds_ptr = n;

	// --------- LEER con WHILE -----------
	unsigned int dato;
	while (!altera_avalon_fifo_read_status(FIFO_OUT_CSR_BASE, ALTERA_AVALON_FIFO_STATUS_E_MSK)) {
		dato = altera_avalon_fifo_read_fifo(FIFO_OUT_BASE, FIFO_OUT_CSR_BASE);
		alt_printf("Leído: %x\n", dato);
		n = altera_avalon_fifo_read_level(FIFO_OUT_CSR_BASE);
		alt_printf("Datos restantes: %x\n", n);
		*leds_ptr = dato;
	}

    alt_printf("FIFO test done.\n");

    while(1);

    return 0;
}
