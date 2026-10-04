#include "recv_packet.hpp"
#include "../crc/crc.h"

gint recv_packet_thread_is_run = FALSE;
gint recv_packet_id = -1;

gchar *filepath = NULL;

int8_t process_recv_packet(uint8_t *data, uint16_t len)
{
	if((data[5] == 0xf1) && (data[6] == 0x03))
	{
		uint32_t addr = ((data[7] << 16)) + ((data[8]) << 8) + data[9];
		if(addr == 0)
		{
			GDateTime *now = g_date_time_new_now_local();
			gchar *filename = g_date_time_format(now, "./fpga_flash/fpga_readback-%Y%m%d_%H%M%S.bin");
			g_date_time_unref(now);

			if (g_mkdir_with_parents("./fpga_flash", 0755) != 0) {
				g_printerr("创建目录失败: %s\n", g_strerror(errno));
				return -1;
			}

			// 例如：serial_log_20260921_143045.bin
			FILE *fp = fopen(filename, "wb");
			if(filepath != NULL)
			{
				g_free(filepath);
			}
			filepath = filename;

			if (fp == NULL) {
				g_printerr("无法打开文件: %s\n", filepath);
				return -2;
			}
			
			gsize written = fwrite(&data[10], 1, (len - 12), fp);
			fflush(fp);  // 确保数据及时写入磁盘
			fclose(fp);
		}
		else 
		{
			FILE *fp = fopen(filepath, "ab");
			if (fp == NULL) {
				g_printerr("无法打开文件: %s\n", filepath);
				return -3;
			}
			gsize written = fwrite(&data[10], 1, (len - 12), fp);
			fflush(fp);  // 确保数据及时写入磁盘
			fclose(fp);
		}
	}
	return 0;
}


uint8_t packet_buf[PACKET_BUFFER_SIZE];
uint16_t packet_buf_waddr = 0;
uint16_t packet_remain = 0;
uint8_t packet_state = 0;
uint8_t packet_id;
uint16_t packet_size;

gpointer recv_packet(gpointer user_data)
{
	while(g_atomic_int_get(&recv_packet_thread_is_run))
	{
		GAsyncQueue *recv_queue = (GAsyncQueue *)user_data;
		// 一次性取出所有积压数据，减少 UI 刷新次数
		GString *batch = g_string_new(NULL);
		SerialChunk *chunk;
		
		while ((chunk = (SerialChunk *)g_async_queue_try_pop(recv_queue)) != NULL) {
			g_string_append_len(batch, (const char *)chunk->data, chunk->length);
			g_free(chunk->data);
			g_free(chunk);
		}
		
		if (batch->len == 0) {
			g_usleep(1000);
		}
		else 
		{
			uint8_t *recv_buf = (uint8_t *)batch->str;
			uint16_t recv_len = batch->len;
			while(recv_len > 0)
			{
				uint16_t i;
				switch (packet_state)
				{
				case 0:
					for(i = 0; i < recv_len; i++)
					{
						if(recv_buf[i] == 0xa0)
							break;
					}
					if(i == recv_len)
					{
						recv_len = 0;
						break;
					}
					else 
					{
						packet_buf[packet_buf_waddr] = recv_buf[i];
						packet_buf_waddr++;
						i++;
						recv_buf += i;
						recv_len -= i;
						packet_state++;
					}
					break;
				
				case 1:
					for(i = 0; i < recv_len; i++)
					{
						if(recv_buf[i] == 0xa5)
							break;
					}
					if(i == recv_len)
					{
						recv_len = 0;
						break;
					}
					else 
					{
						packet_buf[packet_buf_waddr] = recv_buf[i];
						packet_buf_waddr++;
						i++;
						recv_buf += i;
						recv_len -= i;
						packet_state++;
					}
					break;
				
				case 2:
					packet_id = recv_buf[0];
					packet_buf[packet_buf_waddr] = recv_buf[i];
					packet_buf_waddr++;
					recv_buf++;
					recv_len--;
					packet_state++;
					break;
				
				case 3:
					packet_size = (recv_buf[0] << 8);
					packet_buf[packet_buf_waddr] = recv_buf[i];
					packet_buf_waddr++;
					recv_buf++;
					recv_len--;
					packet_state++;
					break;
				
				case 4:
					packet_size += recv_buf[0];
					packet_size += 7;
					if(packet_size <= PACKET_BUFFER_SIZE)
					{
						packet_buf[packet_buf_waddr] = recv_buf[i];
						packet_buf_waddr++;
						packet_remain = packet_size - 5;
						packet_state++;
					}
					else 
					{
						packet_buf_waddr = 0;
						packet_state = 0;
					}
					recv_buf++;
					recv_len--;
					break;
				
				case 5:
					while((recv_len > 0) && (packet_remain > 0))
					{
						uint16_t len = (recv_len > packet_remain) ? packet_remain : recv_len;
						memcpy((packet_buf + packet_buf_waddr), recv_buf, len);
						packet_buf_waddr += len;
						packet_remain -= len;
						recv_buf += len;
						recv_len -= len;
					}
					if(packet_remain == 0)
					{
						uint16_t crc = crc16_ccitt_false(&packet_buf[5], (packet_size - 5), 0xffff);
						if(crc == 0)
						{
							g_atomic_int_set(&recv_packet_id, packet_id);
							process_recv_packet(&packet_buf[0], packet_size);
						}
						packet_buf_waddr = 0;
						packet_state = 0;
					}
					break;
				
				default:
					break;
				}
			}
		}
		g_string_free(batch, TRUE);
	}

	return NULL;
}



