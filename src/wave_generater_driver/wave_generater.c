#include "wave_generater.h"

uint8_t wave_package_id = 0;

uint8_t wave_generater_set_parameter(WAVE_TypeDef *wave, BUFFER_MODE mode, uint8_t update_rate_div, uint32_t sample_points, uint16_t voltage_offset)
{
	uint16_t wave_crc;
	uint8_t tmp_buf[13];
	uint8_t error;
	wave->wave_write_nss(0);
	tmp_buf[0] = 0xa0;
	tmp_buf[1] = 0xa5;
	tmp_buf[2] = wave_package_id;
	wave_package_id++;
	tmp_buf[3] = 8 / 256;
	tmp_buf[4] = 8 % 256;
	tmp_buf[5] = 0x00;
	tmp_buf[6] = mode;
	tmp_buf[7] = update_rate_div;
	tmp_buf[8] = ((sample_points & 0x00ff0000) >> 16);
	tmp_buf[9] = ((sample_points & 0x0000ff00) >> 8);
	tmp_buf[10] = ((sample_points & 0x000000ff) >> 0);
	tmp_buf[11] = ((voltage_offset & 0xff00) >> 8);
	tmp_buf[12] = ((voltage_offset & 0x00ff) >> 0);
	error = wave->wave_transmit(&tmp_buf[0], 13);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave_crc = crc16_ccitt_false(&tmp_buf[5], 8, 0xffff);
	tmp_buf[0] = (wave_crc >> 8);
	tmp_buf[1] = (wave_crc & 0xff);
	error = wave->wave_transmit(&tmp_buf[0], 2);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave->wave_write_nss(1);
	return 0;
}


uint8_t wave_generater_transmit_data(WAVE_TypeDef *wave, uint32_t start_addr, uint8_t *data, uint16_t size)
{
	uint16_t wave_crc;
	uint8_t tmp_buf[9];
	uint8_t error;
	wave->wave_write_nss(0);
	tmp_buf[0] = 0xa0;
	tmp_buf[1] = 0xa5;
	tmp_buf[2] = wave_package_id;
	wave_package_id++;
	tmp_buf[3] = (size + 4) / 256;
	tmp_buf[4] = (size + 4) % 256;
	tmp_buf[5] = 0x01;
	tmp_buf[6] = ((start_addr & 0x00ff0000) >> 16);
	tmp_buf[7] = ((start_addr & 0x0000ff00) >> 8);
	tmp_buf[8] = ((start_addr & 0x000000ff) >> 0);
	error = wave->wave_transmit(&tmp_buf[0], 9);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave_crc = crc16_ccitt_false(&tmp_buf[5], 4, 0xffff);
	error = wave->wave_transmit(&data[0], size);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave_crc = crc16_ccitt_false(&data[0], size, wave_crc);
	tmp_buf[0] = (wave_crc >> 8);
	tmp_buf[1] = (wave_crc & 0xff);
	error = wave->wave_transmit(&tmp_buf[0], 2);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave->wave_write_nss(1);
	return 0;
}


uint8_t wave_generater_set_run(WAVE_TypeDef *wave, uint8_t is_run)
{
	uint16_t wave_crc;
	uint8_t tmp_buf[7];
	uint8_t error;
	wave->wave_write_nss(0);
	tmp_buf[0] = 0xa0;
	tmp_buf[1] = 0xa5;
	tmp_buf[2] = wave_package_id;
	wave_package_id++;
	tmp_buf[3] = 2 / 256;
	tmp_buf[4] = 2 % 256;
	tmp_buf[5] = 0x02;
	tmp_buf[6] = (is_run && 1);
	error = wave->wave_transmit(&tmp_buf[0], 7);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave_crc = crc16_ccitt_false(&tmp_buf[5], 2, 0xffff);
	tmp_buf[0] = (wave_crc >> 8);
	tmp_buf[1] = (wave_crc & 0xff);
	error = wave->wave_transmit(&tmp_buf[0], 2);
	if(error != 0)
	{
		wave->wave_write_nss(1);
		return error;
	}
	wave->wave_write_nss(1);
	return 0;
}




