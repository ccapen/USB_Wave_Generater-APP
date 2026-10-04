#include "update_fpga.hpp"
#include <stdlib.h>
#include <glib.h>


static uint8_t program_packet_id = 0;
extern gint recv_packet_id;


int8_t wait_return(uint8_t id, int32_t timeout_ms)
{
    gint64 start_time = g_get_monotonic_time();
    gint64 timeout_us = (gint64)timeout_ms * 1000;

    while (g_atomic_int_get(&recv_packet_id) != id) {
        // 检查总超时
        gint64 elapsed = g_get_monotonic_time() - start_time;
        if (elapsed >= timeout_us) {
            g_printerr("接收超时! program_packet_id = %d, recv_packet_id = %d;\n", program_packet_id, g_atomic_int_get(&recv_packet_id));
            return -1;
        }
    }
	return 0;
}

int8_t enter_flash_mode(void)
{
	flash_enter_mode(program_packet_id);
	int8_t error = wait_return(program_packet_id, 500);
	if(error != 0)
	{
		g_print("Enter Flash Mode Receive Packet Fail At: %d\n", program_packet_id);
		return -1;
	}
	program_packet_id++;
	return 0;
}

int8_t flash_readback(GSourceFunc update_progress)
{
	uint32_t flash_addr = 0;
	uint32_t total_length = 640000;
	uint32_t length = total_length;
	while(length > 0)
	{
		uint16_t read_len = (length <= 512) ? length : 512;

		flash_read_data(program_packet_id, flash_addr, read_len);
		int8_t error = wait_return(program_packet_id, 500);
		if(error != 0)
		{
			g_print("Readback Receive Packet Fail At: %d\n", program_packet_id);
			return -1;
		}
		program_packet_id++;

		flash_addr += read_len;
		length -= read_len;
		
		float *fraction = new float;
		*fraction = ((float)total_length - (float)length) / (float)total_length;
		g_idle_add(update_progress, fraction);
	}

	g_print("Program Readback Success!\n");
	float *fraction = new float;
	*fraction = 1.0;
	g_idle_add(update_progress, fraction);
	return 0;
}


int8_t flash_update_fpga(char *path, GSourceFunc update_progress)
{
    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;

    // 使用 GLib 读取整个文件（二进制模式）
    if (!g_file_get_contents(path, &contents, &length, &error)) {
        g_printerr("读取文件失败: %s\n", error->message);
        g_error_free(error);
        return -1;
    }
    
    if (contents == NULL) {
		g_printerr("Content Is NULL!\n");
		return -2;
    }

	uint32_t flash_addr = 0;
	uint32_t total_length = length;
	while(length > 0)
	{
		uint16_t send_len = (length <= 4096) ? length : 4096;
		flash_erase_sector(program_packet_id, flash_addr);
		int8_t error = wait_return(program_packet_id, 500);
		if(error != 0)
		{
			g_free(contents);
			g_print("Update FPGA -Erase Receive Packet Fail At: %d\n", program_packet_id);
			return -3;
		}
		program_packet_id++;

		while(send_len > 0)
		{
			uint16_t packet_len = (send_len <= 512) ? send_len : 512;

			flash_write_data(program_packet_id, flash_addr, (uint8_t *)(contents + flash_addr), packet_len);
			int8_t error = wait_return(program_packet_id, 500);
			if(error != 0)
			{
				g_free(contents);
				g_print("Update FPGA -Program Receive Packet Fail At: %d\n", program_packet_id);
				return -4;
			}
			program_packet_id++;

			flash_addr += packet_len;
			send_len -= packet_len;
			length -= packet_len;

			float *fraction = new float;
			*fraction = ((float)total_length - (float)length) / (float)total_length;
			g_idle_add(update_progress, fraction);
		}
	}

	g_free(contents);

	g_print("Update FPGA Success!\n");
	float *fraction = new float;
	*fraction = 1.0;
	g_idle_add(update_progress, fraction);

	return 0;
}





