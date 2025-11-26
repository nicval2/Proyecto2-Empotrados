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

#include "sys/alt_stdio.h"

#define SEGMENTS7_BASE 0x3000
#define TIMER_BASE 0x3020


void display_time_4seg(volatile int *hex_ptr,
                       int minutes, int seconds)
{
    // Tabla 7 segmentos
    int seg7_table[10] = {
        0x3F,0x06,0x5B,0x4F,0x66,
        0x6D,0x7D,0x07,0x7F,0x6F
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


int main()
{ 
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

		  alt_printf("Elapsed ms: %i\n", elapsed_ms);
	  }


  }

  /* Event loop never exits. */
  while (1);

  return 0;
}
