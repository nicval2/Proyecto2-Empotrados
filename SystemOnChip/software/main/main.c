#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>
#include "filter.h"
#include "src/7_segment/7_segment.h"
#include "src/switches/switch.h"

int main()
{
    alt_putstr("System initializing...\n");

    buttons_init();
    switches_init();

    while (1)
    {
        switches_update();
    }

    return 0;
}

