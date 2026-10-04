#include "parse_wsx.hpp"


// 根据进制字符串获取数值
int radix_from_string(const char *str) {
    if (g_strcmp0(str, "BIN") == 0) return 2;
    if (g_strcmp0(str, "OCT") == 0) return 8;
    if (g_strcmp0(str, "DEC") == 0) return 10;
    if (g_strcmp0(str, "HEX") == 0) return 16;
    if (g_strcmp0(str, "UNS") == 0) return 10;
    return -1;
}


int8_t get_header_wsx(char *path, WsxHeader *header)
{
	GError *error = NULL;
    GFile *file = g_file_new_for_path(path);
    GFileInputStream *stream = g_file_read(file, NULL, &error);
    
    if (stream == NULL) {
        g_print("打开失败: %s\n", error->message);
        g_error_free(error);
        g_object_unref(file);
        return -1;
    }
    
    // 用 GDataInputStream 逐行读取
    GDataInputStream *data_stream = g_data_input_stream_new(G_INPUT_STREAM(stream));
    g_data_input_stream_set_newline_type(data_stream, G_DATA_STREAM_NEWLINE_TYPE_ANY);
    
	// GtkTextBuffer buffer;
    char *line = NULL;
    gsize length = 0;
    int line_num = 0;
	gboolean in_comment = FALSE;
	uint8_t header_fill = 0;
    
    while ((line = g_data_input_stream_read_line(data_stream, &length, NULL, &error)) != NULL)
	{
        line_num++;
		
		if(in_comment)
		{
			char *end_comment = strstr(line, "*/");
			if (end_comment) {
				memmove(line, end_comment + 2, strlen(end_comment + 2) + 1);
				in_comment = FALSE;
			} else {
				*line = '\0';
			}
		}
		else 
		{
			char *start_comment = strstr(line, "/*");
			if (start_comment) {
				char *end_comment = strstr(line, "*/");
				if (end_comment) {
					memmove(start_comment, end_comment + 2, strlen(end_comment + 2) + 1);
				} else {
					*start_comment = '\0';
					in_comment = TRUE;
				}
			} 
		}

		char *clean_line = g_strstrip(line);
		// 跳过空行和注释
		if ((*clean_line == '\0') || (g_str_has_prefix(clean_line, "//"))) 
		{
			g_free(line);
			continue;
		}

		char *pos = strchr(clean_line, '=');
		if (pos != NULL) {
			*pos = '\0';
			char *key = g_strstrip(clean_line);
			char *value_str = g_strstrip(pos + 1);
			
			// 去掉末尾分号
			char *semicolon = strchr(value_str, ';');
			if(semicolon != NULL)
			{
				*semicolon = '\0';
			}
			else 
			{
				g_printerr("End Semicolon Missed At: %s\n", line);
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -2;
			}
			g_strstrip(value_str);
			
			// 转换为数字
			char *endptr = NULL;
			errno = 0;
			long value = strtol(value_str, &endptr, 10);
			
			if (endptr == value_str) {
				if(g_strcmp0(key, "ADDRESS_RADIX") == 0)
				{
					header->addr_radix = radix_from_string(value_str);
					header_fill += 8;
				}
				else if(g_strcmp0(key, "DATA_RADIX") == 0)
				{
					header->data_radix = radix_from_string(value_str);
					header_fill += 16;
				}
				else 
				{
					g_printerr("'%s' 不是有效数字\n", value_str);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -3;
				}
			} else if (*endptr != '\0') {
				g_printerr("'%s' 包含非数字字符\n", value_str);
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -4;
			} else if (errno == ERANGE) {
				g_printerr("数值超出范围\n");
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -5;
			} else {
				if(g_strcmp0(key, "SAMPLE_RATE") == 0)
				{
					header->sample_rate = value;
					header_fill += 1;
				}
				else if(g_strcmp0(key, "SAMPLE_POINTS") == 0)
				{
					header->sample_points = value;
					header_fill += 2;
				}
				else if(g_strcmp0(key, "VOLTAGE_OFFSET") == 0)
				{
					header->voltage_offset = value;
					header_fill += 4;
				}
				else 
				{
					g_printerr("KEY Unfound!: %s\n", value_str);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -6;
				}
			}
			if(header_fill == 0b11111)
			{
				if(header->sample_rate > 125000000)
				{
					g_printerr("SAMPLE_RATE Too Big!: %d\n", header->sample_rate);
				}
				else if(header->sample_points > 4194304)
				{
					g_printerr("SAMPLE_POINTS Too Big!: %d\n", header->sample_points);
				}
				else if((header->voltage_offset > 511) || (header->voltage_offset < -512))
				{
					g_printerr("VOLTAGE_OFFSET Out Of Range!: %d\n", header->voltage_offset);
				}
				else if((header->addr_radix == -1) || (header->data_radix == -1))
				{
					g_printerr("RADIX Match Fail!: %d:%d\n", header->addr_radix, header->data_radix);
				}
				else 
				{
					g_free(line);
					break;
				}
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -7;
			}
		}
		else 
		{
			g_printerr("Read Header Error!: %s\n", line);
			g_free(line);
			g_object_unref(data_stream);
			g_object_unref(stream);
			g_object_unref(file);
			return -8;
		}

        g_free(line);
    }
    
    if (error != NULL) {
        g_print("读取错误: %s\n", error->message);
        g_error_free(error);
    }

	g_print("Get WSX Header Success!\n");
    
    g_object_unref(data_stream);
    g_object_unref(stream);
    g_object_unref(file);

	return 0;
}


int8_t parse_wsx(char *path, GSourceFunc update_progress)
{
	GError *error = NULL;
    GFile *file = g_file_new_for_path(path);
    GFileInputStream *stream = g_file_read(file, NULL, &error);
    
    if (stream == NULL) {
        g_print("打开失败: %s\n", error->message);
        g_error_free(error);
        g_object_unref(file);
        return -1;
    }
    
    // 用 GDataInputStream 逐行读取
    GDataInputStream *data_stream = g_data_input_stream_new(G_INPUT_STREAM(stream));
    g_data_input_stream_set_newline_type(data_stream, G_DATA_STREAM_NEWLINE_TYPE_ANY);
    
	// GtkTextBuffer buffer;
    char *line = NULL;
    gsize length = 0;
    int line_num = 0;
	gboolean in_comment = FALSE;
	uint8_t state = 0;
	WsxHeader header;
	uint8_t header_fill = 0;

	#define PACKET_NUM	500
	int16_t data[PACKET_NUM];
	uint32_t point_addr = 0;
	
	wave_generater_set_run(&wave1, 0);
	g_usleep(10000);
	wave_generater_set_run(&wave1, 0);
    while ((line = g_data_input_stream_read_line(data_stream, &length, NULL, &error)) != NULL)
	{
        line_num++;
		
		if(in_comment)
		{
			char *end_comment = strstr(line, "*/");
			if (end_comment) {
				memmove(line, end_comment + 2, strlen(end_comment + 2) + 1);
				in_comment = FALSE;
			} else {
				*line = '\0';
			}
		}
		else 
		{
			char *start_comment = strstr(line, "/*");
			if (start_comment) {
				char *end_comment = strstr(line, "*/");
				if (end_comment) {
					memmove(start_comment, end_comment + 2, strlen(end_comment + 2) + 1);
				} else {
					*start_comment = '\0';
					in_comment = TRUE;
				}
			} 
		}

		char *clean_line = g_strstrip(line);
		// 跳过空行和注释
		if ((*clean_line == '\0') || (g_str_has_prefix(clean_line, "//"))) 
		{
			g_free(line);
			continue;
		}

		switch (state)
		{
		case 0:{
			char *pos = strchr(clean_line, '=');
			if (pos != NULL) {
				*pos = '\0';
				char *key = g_strstrip(clean_line);
				char *value_str = g_strstrip(pos + 1);
				
                // 去掉末尾分号
				char *semicolon = strchr(value_str, ';');
				if(semicolon != NULL)
				{
					*semicolon = '\0';
				}
				else 
				{
					g_printerr("End Semicolon Missed At: %s\n", line);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -2;
				}
                g_strstrip(value_str);
				
				// 转换为数字
				char *endptr = NULL;
				errno = 0;
				long value = strtol(value_str, &endptr, 10);
				
				if (endptr == value_str) {
					if(g_strcmp0(key, "ADDRESS_RADIX") == 0)
					{
						header.addr_radix = radix_from_string(value_str);
						header_fill += 8;
					}
					else if(g_strcmp0(key, "DATA_RADIX") == 0)
					{
						header.data_radix = radix_from_string(value_str);
						header_fill += 16;
					}
					else 
					{
						g_printerr("'%s' 不是有效数字\n", value_str);
						g_free(line);
						g_object_unref(data_stream);
						g_object_unref(stream);
						g_object_unref(file);
						return -3;
					}
				} else if (*endptr != '\0') {
					g_printerr("'%s' 包含非数字字符\n", value_str);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -4;
				} else if (errno == ERANGE) {
					g_printerr("数值超出范围\n");
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -5;
				} else {
					if(g_strcmp0(key, "SAMPLE_RATE") == 0)
					{
						header.sample_rate = value;
						header_fill += 1;
					}
					else if(g_strcmp0(key, "SAMPLE_POINTS") == 0)
					{
						header.sample_points = value;
						header_fill += 2;
					}
					else if(g_strcmp0(key, "VOLTAGE_OFFSET") == 0)
					{
						header.voltage_offset = value;
						header_fill += 4;
					}
					else 
					{
						g_printerr("KEY Unfound!: %s\n", value_str);
						g_free(line);
						g_object_unref(data_stream);
						g_object_unref(stream);
						g_object_unref(file);
						return -6;
					}
				}
				if(header_fill == 0b11111)
				{
					if((header.sample_rate > 125000000) || (header.sample_rate < 488281))
					{
						g_printerr("SAMPLE_RATE Out Of Range!: %d\n", header.sample_rate);
					}
					else if(header.sample_points > 4194304)
					{
						g_printerr("SAMPLE_POINTS Too Big!: %d\n", header.sample_points);
					}
					else if((header.voltage_offset > 511) || (header.voltage_offset < -512))
					{
						g_printerr("VOLTAGE_OFFSET Out Of Range!: %d\n", header.voltage_offset);
					}
					else if((header.addr_radix == -1) || (header.data_radix == -1))
					{
						g_printerr("RADIX Match Fail!: %d:%d\n", header.addr_radix, header.data_radix);
					}
					else 
					{
						uint8_t updata_rate_div = (125000000 / header.sample_rate) - 1;
						wave_generater_set_parameter(&wave1, LOOP, updata_rate_div, header.sample_points, header.voltage_offset);
						state++;
						break;
					}
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -7;
				}
			}
			else 
			{
				g_printerr("Read Header Error!: %s\n", line);
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -8;
			}
			break;
		}
		case 1:{
			if(g_str_has_prefix(clean_line, "DATA"))
			{
				state++;
			}
			break;
		}
		case 2:{
			if(g_str_has_prefix(clean_line, "BEGIN"))
			{
				state++;
			}
			break;
		}
		case 3:{
			if(g_str_has_prefix(clean_line, "END"))
			{
				if(point_addr == header.sample_points)
				{
					if((point_addr % PACKET_NUM) != 0)
					{
					wave_generater_transmit_points(&wave1, (point_addr - (point_addr % PACKET_NUM)), data, (point_addr % PACKET_NUM));
					}
					state++;
					break;
				}
				else 
				{
					g_printerr("END Without Enough Data At: %d\n", point_addr);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -9;
				}
			}
            // 解析数据行：地址 : 数据;
            char *colon = strchr(clean_line, ':');
            if (colon != NULL) {
                *colon = '\0';
                char *addr_str = g_strstrip(clean_line);
                char *data_str = g_strstrip(colon + 1);
                
                // 去掉末尾分号
				char *semicolon = strchr(data_str, ';');
				if(semicolon != NULL)
				{
					*semicolon = '\0';
				}
				else 
				{
					g_printerr("End Semicolon Missed At: %s\n", line);
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -10;
				}
                g_strstrip(data_str);
				
                // 处理地址范围 [start..end] 或单个地址
                guint32 addr_start, addr_end;
                if (addr_str[0] == '[') {
                    // [start..end]
                    char *dotdot = strstr(addr_str, "..");
                    if (dotdot) {
                        *dotdot = '\0';
                        char *start_s = g_strstrip(addr_str + 1);
                        char *end_s = g_strstrip(dotdot + 2);
                        // 去掉 ]
                        int elen = strlen(end_s);
                        if (elen > 0 && end_s[elen-1] == ']') end_s[elen-1] = '\0';
                        
                        addr_start = strtol(start_s, NULL, header.addr_radix);
                        addr_end = strtol(end_s, NULL, header.addr_radix);
                    } else {
                        addr_start = addr_end = 0;
                    }
                } else {
                    addr_start = addr_end = strtol(addr_str, NULL, header.addr_radix);
                }
				
                // 解析数据值（可能多个：8 : F E 5）
                gint32 values[256];
                int value_count = 0;
                char **tokens = g_strsplit(data_str, " ", -1);
                for (int t = 0; tokens[t] != NULL && value_count < 256; t++) {
                    char *tok = g_strstrip(tokens[t]);
                    if (*tok == '\0') continue;
                    values[value_count++] = strtol(tok, NULL, header.data_radix);
                }
                g_strfreev(tokens);
                
                if (value_count == 0)
				{
					break;
				}
				
                // 填充数据
                guint32 addr = addr_start;
                int vi = 0;
                while (addr <= addr_end) {
					if(addr != point_addr)
					{
						g_printerr("Ponit Addr Not Match At: %d\n", point_addr);
					}
					else if(point_addr >= header.sample_points)
					{
						g_printerr("Data Beyond Points Number At: %d\n", point_addr);
					}
					else if((values[vi % value_count] > 511) || (values[vi % value_count] < -512))
					{
						g_printerr("Data Out Of Range At: %d:%d\n", point_addr, values[vi % value_count]);
					}
					else 
					{
						data[point_addr % PACKET_NUM] = values[vi % value_count];
						point_addr++;
						if((point_addr % PACKET_NUM) == 0)
						{
							wave_generater_transmit_points(&wave1, (point_addr - PACKET_NUM), data, PACKET_NUM);
							float *fraction = new float;
							*fraction = (float)point_addr / header.sample_points;
							g_idle_add(update_progress, fraction);
							// update_progress((float)point_addr / header.sample_points);
						}
						addr++;
						vi++;
						continue;
					}
					g_free(line);
					g_object_unref(data_stream);
					g_object_unref(stream);
					g_object_unref(file);
					return -11;
                }
			}
			else 
			{
				g_printerr("DATA Content Format Error At: %d\n", point_addr);
				g_free(line);
				g_object_unref(data_stream);
				g_object_unref(stream);
				g_object_unref(file);
				return -12;
			}
			break;
		}
		case 4:
			break;
		
		default:
			break;
		}

        g_free(line);
    }
    
    if (error != NULL) {
        g_print("读取错误: %s\n", error->message);
        g_error_free(error);
		g_object_unref(data_stream);
		g_object_unref(stream);
		g_object_unref(file);
		return -13;
    }

	wave_generater_set_run(&wave1, TRUE);
	g_print("Parse WSX Success!\n");
	float *fraction = new float;
	*fraction = 1.0;
	g_idle_add(update_progress, fraction);
	// update_progress(1.0);
    
    g_object_unref(data_stream);
    g_object_unref(stream);
    g_object_unref(file);

	return 0;
}
