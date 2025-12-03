#ifndef UI_H
#define UI_H

#include <stdint.h>

void ui_init(void);
void ui_show_waiting(void);
void ui_show_now_playing(void);
void ui_update_timer(uint32_t seconds);
void ui_update_filter(void);
void ui_transition(void);

#endif
