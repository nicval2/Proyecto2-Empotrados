#include "timer.h"
#include "system.h"

// Timer Register Map (Offsets en words de 16 bits o 32 bits según config)
// Avalon Timer Standard:
// 0: Status, 1: Control, 2: PeriodL, 3: PeriodH, 4: SnapL, 5: SnapH
#define TIMER_PTR ((volatile unsigned short *)TIMER_BASE)

void timer_init(void)
{
    // Control register = 0x0 (Stop), limpiar Status
    TIMER_PTR[1] = 0x8; // STOP y limpiar TO
    TIMER_PTR[0] = 0;   // Status clear

    // Configurar START | CONT (0x4 | 0x2 = 0x6)
    TIMER_PTR[1] = 0x6;
}

int timer_tick(void)
{
    // Leer Status register (offset 0)
    int st = TIMER_PTR[0];

    if (st & 1) // TO bit
    {
        TIMER_PTR[0] = 0; // Clear TO
        return 1;
    }
    return 0;
}
