// include/sd_manager.h
#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "audio_filter.h" // Necesita la definición de audio_buffer_t

int SD_MANAGER_init(const char *filename);
int SD_MANAGER_read_audio_chunk(audio_buffer_t *buffer);
void SD_MANAGER_rewind();

#endif // SD_MANAGER_H
