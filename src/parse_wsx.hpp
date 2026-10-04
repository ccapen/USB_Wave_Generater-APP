#ifndef _PARSE_WSX_
#define _PARSE_WSX_

#include <gtk/gtk.h>
#include "wave_generater_driver/wave_generater_gtk.hpp"


typedef struct {
	uint32_t sample_rate;
	uint32_t sample_points;
	int16_t voltage_offset;
    int8_t addr_radix;
    int8_t data_radix;
} WsxHeader;


int8_t get_header_wsx(char *path, WsxHeader *header);
int8_t parse_wsx(char *path, GSourceFunc update_progress);



#endif
