#ifndef _FLASH_DRIVER_H_
#define _FLASH_DRIVER_H_

#include <stdint.h>




uint8_t flash_enter_mode(uint8_t packet_id);
uint8_t flash_read_data(uint8_t packet_id, uint32_t addr, uint16_t len);
uint8_t flash_erase_sector(uint8_t packet_id, uint32_t addr);
uint8_t flash_erase_half_block(uint8_t packet_id, uint32_t addr);
uint8_t flash_erase_full_block(uint8_t packet_id, uint32_t addr);
uint8_t flash_write_data(uint8_t packet_id, uint32_t addr, uint8_t *data, uint16_t len);


#endif
