#ifndef _WAVE_GENERATER_STM_
#define _WAVE_GENERATER_STM_

#include "wave_generater.h"
#include <gtk/gtk.h>
#include <libserialport.h>


extern WAVE_TypeDef wave1;

uint8_t wave_generater_init(void);
uint8_t wave_generater_transmit_points(WAVE_TypeDef *wave, uint32_t start_addr, int16_t *points, uint16_t num);

#endif
