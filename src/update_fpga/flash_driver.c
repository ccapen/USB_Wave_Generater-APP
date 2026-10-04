#include "flash_driver.h"

#include <libserialport.h>
#include <gtk/gtk.h>
#include "../crc/crc.h"
#include <stdlib.h>
#include <string.h>


extern struct sp_port *selected_port;


uint8_t flash_transmit(uint8_t *pdata, uint16_t size)
{
    if (selected_port == NULL) {
        g_printerr("串口未打开，无法发送数据\n");
        return -1;
    }
    
    // 在工作线程中，可以安全地使用阻塞写入
    int bytes_written = sp_blocking_write(selected_port, pdata, size, 5000);
    
    if (bytes_written == size) {
        g_print("数据全部发送完成: %d bytes\n", bytes_written);
		return 0;
    } else if (bytes_written < 0) {
        // 发送失败
        char *err = sp_last_error_message();
        g_printerr("发送失败: %s\n", err);
        sp_free_error_message(err);
		return -1;
    } else {
        // 部分发送（通常因为超时）
        g_printerr("仅发送了 %d / %zd 字节\n", bytes_written, size);
		return -1;
    }
}

uint8_t flash_enter_mode(uint8_t packet_id)
{
	uint8_t buf[9];
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = 0x00;
	buf[4] = 0x02;
	buf[5] = 0xf0;
	buf[6] = 0x01;	//is_program_mode
	uint16_t crc = crc16_ccitt_false(&buf[5], 2, 0xffff);
	buf[7] = ((crc >> 8) & 0xff);
	buf[8] = (crc & 0xff);

	flash_transmit(&buf[0], 9);
	return 0;
}

uint8_t flash_read_data(uint8_t packet_id, uint32_t addr, uint16_t len)
{
	uint8_t buf[14];
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = 0x00;
	buf[4] = 0x07;
	buf[5] = 0xf1;
	buf[6] = 0x03;	//read
	buf[7] = ((addr >> 16) & 0xff);
	buf[8] = ((addr >> 8) & 0xff);
	buf[9] = ((addr >> 0) & 0xff);
	buf[10] = ((len >> 8) & 0xff);
	buf[11] = (len & 0xff);
	uint16_t crc = crc16_ccitt_false(&buf[5], 7, 0xffff);
	buf[12] = ((crc >> 8) & 0xff);
	buf[13] = (crc & 0xff);

	flash_transmit(&buf[0], 14);
	return 0;
}

uint8_t flash_erase_sector(uint8_t packet_id, uint32_t addr)
{
	uint8_t buf[12];
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = 0x00;
	buf[4] = 0x05;
	buf[5] = 0xf1;
	buf[6] = 0x20;	//sector erase(4kbytes)
	buf[7] = ((addr >> 16) & 0xff);
	buf[8] = ((addr >> 8) & 0xff);
	buf[9] = ((addr >> 0) & 0xff);
	uint16_t crc = crc16_ccitt_false(&buf[5], 5, 0xffff);
	buf[10] = ((crc >> 8) & 0xff);
	buf[11] = (crc & 0xff);

	flash_transmit(&buf[0], 12);
	return 0;
}

uint8_t flash_erase_half_block(uint8_t packet_id, uint32_t addr)
{
	uint8_t buf[12];
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = 0x00;
	buf[4] = 0x05;
	buf[5] = 0xf1;
	buf[6] = 0x52;	//block erase(32kbytes)
	buf[7] = ((addr >> 16) & 0xff);
	buf[8] = ((addr >> 8) & 0xff);
	buf[9] = ((addr >> 0) & 0xff);
	uint16_t crc = crc16_ccitt_false(&buf[5], 5, 0xffff);
	buf[10] = ((crc >> 8) & 0xff);
	buf[11] = (crc & 0xff);

	flash_transmit(&buf[0], 12);
	return 0;
}

uint8_t flash_erase_full_block(uint8_t packet_id, uint32_t addr)
{
	uint8_t buf[12];
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = 0x00;
	buf[4] = 0x05;
	buf[5] = 0xf1;
	buf[6] = 0xd8;	//block erase(64kbytes)
	buf[7] = ((addr >> 16) & 0xff);
	buf[8] = ((addr >> 8) & 0xff);
	buf[9] = ((addr >> 0) & 0xff);
	uint16_t crc = crc16_ccitt_false(&buf[5], 5, 0xffff);
	buf[10] = ((crc >> 8) & 0xff);
	buf[11] = (crc & 0xff);

	flash_transmit(&buf[0], 12);
	return 0;
}

uint8_t flash_write_data(uint8_t packet_id, uint32_t addr, uint8_t *data, uint16_t len)
{
	if(len > 512)
	{
		g_print("Flash Write Length Too Big!\n");
		return -1;
	}
	
	uint8_t *buf = (uint8_t *)malloc(len + 12);
	
	buf[0] = 0xa0;
	buf[1] = 0xa5;
	buf[2] = packet_id;
	buf[3] = (((len + 5) >> 8) & 0xff);
	buf[4] = (((len + 5) >> 0) & 0xff);
	buf[5] = 0xf1;
	buf[6] = 0x02;	//program
	buf[7] = ((addr >> 16) & 0xff);
	buf[8] = ((addr >> 8) & 0xff);
	buf[9] = ((addr >> 0) & 0xff);
	memcpy(&buf[10], data, len);
	uint16_t crc = crc16_ccitt_false(&buf[5], (len + 5), 0xffff);
	buf[len + 10] = ((crc >> 8) & 0xff);
	buf[len + 11] = (crc & 0xff);

	flash_transmit(&buf[0], len + 12);
	free(buf);
	return 0;
}




