#ifndef SWITCH_H_
#define SWITCH_H_

#include <stdint.h>

// Buttons (REG_BUTTONS_BASE is defined in system.h)
void buttons_init();

// Switches (REG_SWITCHES_BASE is defined in system.h)
void switches_init();
void switches_update();

// Optional: retrieve last switch value
int switches_get_value();

#endif
