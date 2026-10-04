#include "wave_generater_gtk.hpp"
#include <string>

WAVE_TypeDef wave1 = {0};

extern struct sp_port *selected_port;

extern std::string gbk_to_utf8(const std::string& gbk_str);

//CALLBACK

uint8_t wave_write_nss(uint8_t pin_state)
{
	return 0;
}

uint8_t wave_transmit(uint8_t *pdata, uint16_t size)
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
        // char *err = sp_last_error_message();
		std::string str = gbk_to_utf8(sp_last_error_message());
        g_printerr("发送失败: %s\n", str.c_str());
        // sp_free_error_message(err);
		return -1;
    } else {
        // 部分发送（通常因为超时）
        g_printerr("仅发送了 %d / %zd 字节\n", bytes_written, size);
		return -1;
    }
}

//END CALLBACK


uint8_t wave_generater_init(void)
{
	wave1.wave_write_nss = wave_write_nss;
	wave1.wave_transmit = wave_transmit;

	return 0;
}

static inline int is_little_endian(void) {
    uint16_t x = 0x0001;
    return *(uint8_t*)&x == 0x01;   // 低地址是 0x01 → 小端
}

uint8_t wave_generater_transmit_points(WAVE_TypeDef *wave, uint32_t start_addr, int16_t *points, uint16_t num)
{
	if(is_little_endian())
	{
		uint8_t* data = new uint8_t[num * 2];
		for(uint16_t i = 0; i < num; i++)
		{
			data[i * 2] = *(points + i) >> 8;
			data[i * 2 + 1] = *(points + i) & 0xff;
		}
		wave_generater_transmit_data(&wave1, start_addr, data, num * 2);
		delete[] data;
	}
	else 
	{
		wave_generater_transmit_data(&wave1, start_addr, (uint8_t *)points, num * 2);
	}
	return 0;
}


