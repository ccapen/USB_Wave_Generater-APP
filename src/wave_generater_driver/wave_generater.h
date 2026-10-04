#ifndef _WAVE_GENERATER_
#define _WAVE_GENERATER_

#include <stdint.h>
#include "../crc/crc.h"



typedef uint8_t (*WAVE_WRITE_NSS_TypeDef)(uint8_t pin_state);
typedef uint8_t (*WAVE_TRANSMIT_TypeDef)(uint8_t *pdata, uint16_t size);


typedef struct
{
	WAVE_WRITE_NSS_TypeDef wave_write_nss;
	WAVE_TRANSMIT_TypeDef  wave_transmit;
}WAVE_TypeDef;

typedef enum
{
	LOOP = 0,
	STREAM = 1
}BUFFER_MODE;



uint8_t wave_generater_set_parameter(WAVE_TypeDef *wave, BUFFER_MODE mode, uint8_t update_rate_div, uint32_t sample_points, uint16_t voltage_offset);
uint8_t wave_generater_transmit_data(WAVE_TypeDef *wave, uint32_t start_addr, uint8_t *data, uint16_t size);
uint8_t wave_generater_set_run(WAVE_TypeDef *wave, uint8_t is_run);



#endif