#ifndef _UPDATE_FPGA_H_
#define _UPDATE_FPGA_H_


#include <stdint.h>
#include <gtk/gtk.h>
#include "flash_driver.h"





int8_t enter_flash_mode(void);
int8_t flash_readback(GSourceFunc update_progress);
int8_t flash_update_fpga(char *path, GSourceFunc update_progress);




#endif
