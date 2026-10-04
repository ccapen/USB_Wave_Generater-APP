#include <gtk/gtk.h>

#include <libserialport.h>
#include "general_struct.hpp"
#include "parse_wsx.hpp"
#include "update_fpga/update_fpga.hpp"
#include "update_fpga/recv_packet.hpp"
// #include <windows.h>
// #include <io.h>      // for _open_osfhandle
// #include <fcntl.h>   // for _O_RDWR
// #include <glib.h>
// #include <gio/gio.h>

#include <string.h>
#include "text_view_log.hpp"

#include <glibmm/convert.h>
#include <glibmm/ustring.h>

#include <glib.h>
#include <string>
#include <stdexcept>

#include <sys/time.h>  // 用于精确计时

std::string gbk_to_utf8(const std::string& gbk_str) {
	try {
		// Glib::convert 会返回一个 UTF-8 编码的 std::string
		std::string utf8_string = Glib::convert(gbk_str, "UTF-8", "GBK");
		return utf8_string;

		// 直接构造 Glib::ustring，这是 GTK 界面最友好的字符串类型
		// Glib::ustring display_text = utf8_string; 

		// 或者直接一步到位，但注意构造函数的参数顺序
		// Glib::ustring ustr = Glib::convert(gbk_input, "UTF-8", "GBK"); 
		
		// 现在可以将 display_text 用于任何 GTK 控件，比如 gtk_label_set_text()
	} catch (const Glib::ConvertError& ex) {
		// 处理转换异常
		g_printerr("Encoding conversion failed: %s\n", ex.what().c_str());
	}
	return std::string(0);
}

// 比较两个 GtkStringList 是否相等
static gboolean string_lists_equal(GtkStringList *list1, GtkStringList *list2) {
    // 1. 检查是否为 NULL
    if (list1 == NULL && list2 == NULL) return TRUE;
    if (list1 == NULL || list2 == NULL) return FALSE;
    
    // 2. 比较长度
    guint len1 = g_list_model_get_n_items(G_LIST_MODEL(list1));
    guint len2 = g_list_model_get_n_items(G_LIST_MODEL(list2));
    
    if (len1 != len2) return FALSE;
    
    // 3. 逐个比较每个字符串
    for (guint i = 0; i < len1; i++) {
        const char *str1 = gtk_string_list_get_string(list1, i);
        const char *str2 = gtk_string_list_get_string(list2, i);
        
        // 使用 g_strcmp0 安全比较（可以处理 NULL）
        if (g_strcmp0(str1, str2) != 0) {
            return FALSE;
        }
    }
    
    return TRUE;
}

// std::string gbk_to_utf8(const std::string& gbk_str) {
//     GError* error = nullptr;
//     // 关键：指定 from 为 "GBK"，to 为 "UTF-8"
//     gchar* utf8_cstr = g_convert(gbk_str.c_str(), 
//                                  gbk_str.size(), 
//                                  "UTF-8",    // 目标编码
//                                  "GBK",      // 源编码
//                                  nullptr,    // bytes_read
//                                  nullptr,    // bytes_written
//                                  &error);

//     if (error) {
//         std::string err_msg = error->message;
//         g_error_free(error);
//         throw std::runtime_error("GBK to UTF-8 conversion failed: " + err_msg);
//     }

//     std::string utf8_str(utf8_cstr);
//     g_free(utf8_cstr);
//     return utf8_str;
// }

// char tmp_s[200];

// void list_ports(char *s) { else {
// 		strcpy(&s[0], "No serial devices detected\n");
//         // std::cout << "No serial devices detected\n";
//     }
// }


// 获取当前微秒时间戳
static gint64 get_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (gint64)tv.tv_sec * 1000000 + tv.tv_usec;
}


// GIOChannel* create_serial_io_channel(struct sp_port *port)
// {
//     GIOChannel *channel = NULL;
    
//     if (port == NULL) {
//         return NULL;
//     }
    
// #ifdef _WIN32
//     // Windows 平台：使用 Win32 专用 API
//     HANDLE hCom;
// 	sp_get_port_handle(port, &hCom);
//     if (hCom == INVALID_HANDLE_VALUE) {
//         g_printerr("获取串口句柄失败\n");
//         return NULL;
//     }
//     // 将 HANDLE 转换为 CRT 文件描述符
//     int c_fd = _open_osfhandle((intptr_t)hCom, _O_RDWR | _O_BINARY);
//     if (c_fd == -1) {
//         g_printerr("转换为 CRT 文件描述符失败\n");
//         return NULL;
//     }
    
//     channel = g_io_channel_win32_new_fd(c_fd);
// #else
//     // Linux/Unix 平台
//     int fd = sp_get_port_handle(port);
//     if (fd < 0) {
//         g_printerr("获取串口文件描述符失败\n");
//         return NULL;
//     }
//     channel = g_io_channel_unix_new(fd);
// #endif
    
//     if (channel != NULL) {
//         // 设置为原始二进制模式，不做字符编码转换
//         g_io_channel_set_encoding(channel, NULL, NULL);
//         // 设置缓冲区大小为 0，实时传输
//         g_io_channel_set_buffered(channel, FALSE);
//     }
    
//     return channel;
// }


static LogManager *lm;

// // 添加日志消息
// void add_log(const char *text) {
//     GtkTextIter end;

//     // 追加新文本
//     gtk_text_buffer_get_end_iter(log_buffer, &end);
//     gtk_text_buffer_insert(log_buffer, &end, text, -1);
	
//     // 滚动到最新位置
//     gtk_text_buffer_get_end_iter(log_buffer, &end);
//     gtk_text_view_scroll_to_iter(log_view, &end, 0.0, TRUE, 0.0, 0.0);
// }

static void print_hello (GtkWidget *widget, gpointer   data)
{
  log_printf(lm, "Hello World");
}


static void on_file_dialog_finish(GObject *source_object, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GtkWidget *entry = GTK_WIDGET(user_data);
    GError *error = NULL;
    
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    
    if (file != NULL) {
        char *path = g_file_get_path(file);
        gtk_editable_set_text(GTK_EDITABLE(entry), path);
        g_free(path);
        g_object_unref(file);
    }
    
    if (error != NULL) g_error_free(error);
}

// 图标点击回调函数
static void on_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos, gpointer user_data) {
    GtkWidget *window = gtk_widget_get_ancestor(GTK_WIDGET(entry), GTK_TYPE_WINDOW);
    
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "选择文件");
    
    // 1. 创建第一个过滤器：波形文件
    GtkFileFilter *filter_wave = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_wave, "波形文件 (*.wsx)");
    gtk_file_filter_add_suffix(filter_wave, "wsx"); // 按后缀过滤[citation:2]

    // 1. 创建第二个过滤器：更新文件
    GtkFileFilter *filter_update = gtk_file_filter_new();
    // gtk_file_filter_set_name(filter_update, "更新文件 (*.bit *.bin)");
    gtk_file_filter_set_name(filter_update, "更新文件 (*.bin)");
    // gtk_file_filter_add_suffix(filter_update, "bit"); // 按后缀过滤[citation:2]
    gtk_file_filter_add_suffix(filter_update, "bin"); // 按后缀过滤[citation:2]

    // 2. 创建第三个过滤器：所有文件
    GtkFileFilter *filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "所有文件");
    gtk_file_filter_add_pattern(filter_all, "*"); // 匹配所有文件[citation:4]

    // 3. 将过滤器添加到 GListStore 中
    GListStore *filter_list = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filter_list, filter_wave);
    g_list_store_append(filter_list, filter_update);
    g_list_store_append(filter_list, filter_all);

    // 4. 将列表模型设置到对话框中
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filter_list));

    // 5. 设置默认选中的过滤器（可选）
    gtk_file_dialog_set_default_filter(dialog, filter_wave);

    // 释放本地引用（对话框会持有其引用）
    g_object_unref(filter_wave);
    g_object_unref(filter_update);
    g_object_unref(filter_all);
    g_object_unref(filter_list);

    gtk_file_dialog_open(dialog,
                         GTK_WINDOW(window),
                         NULL,
                         on_file_dialog_finish,
                         entry);
    // // 当点击右侧图标时，清空输入框内容
    // if (icon_pos == GTK_ENTRY_ICON_SECONDARY) {
	// 	gtk_editable_set_text(GTK_EDITABLE(entry), "Hello");
    //     g_print("已清空输入框\n");
    // }
}


gboolean is_keep_scan = TRUE;

gboolean update_dropdown(gpointer user_data)
{
    GtkDropDown *dropdown = GTK_DROP_DOWN(user_data);
	GtkSortListModel *model = GTK_SORT_LIST_MODEL(gtk_drop_down_get_model(dropdown));
	GtkStringList *list = GTK_STRING_LIST(gtk_sort_list_model_get_model(model));

	if(is_keep_scan != TRUE)
	{
		return TRUE;
	}

	GtkStringList *ports_list = gtk_string_list_new(NULL);
	struct sp_port **ports;
	// 调用 sp_list_ports 获取端口列表
	sp_return error = sp_list_ports(&ports);
	if (error == SP_OK) {
		for (int i = 0; ports[i]; i++) {
			// 打印每个端口的名称
			char *s = sp_get_port_description(ports[i]);
			if(s == NULL)
			{
				continue;
			}
			std::string str = gbk_to_utf8(s);
			gtk_string_list_append(ports_list, str.c_str());
		}
		// 释放端口列表内存
		sp_free_port_list(ports);
	}

	if(string_lists_equal(list, ports_list))
	{
		return TRUE;
	}

    // 1. 保存当前选中的项目对象
    const char *selected_text = NULL;
	guint selected_pos = gtk_drop_down_get_selected(dropdown);
	if (selected_pos != GTK_INVALID_LIST_POSITION) {
		selected_text = gtk_string_list_get_string(list, selected_pos);
		// 复制字符串，因为后面可能被释放
		selected_text = g_strdup(selected_text);
	}

	// 清空所有项
	gtk_string_list_splice(list, 0, g_list_model_get_n_items(G_LIST_MODEL(list)), NULL);
	// 1. 遍历源列表，逐个追加到目标列表
	guint n = g_list_model_get_n_items(G_LIST_MODEL(ports_list));
	for (guint i = 0; i < n; i++) {
		const char *str = gtk_string_list_get_string(ports_list, i);
		gtk_string_list_append(list, str);   // ✅ 追加到目标列表
	}
	// 替换模型
	// gtk_drop_down_set_model(dropdown, G_LIST_MODEL(ports_list));
	g_object_unref(ports_list);

    // 4. 恢复选中状态
    if (selected_text != NULL) {
        guint new_pos = GTK_INVALID_LIST_POSITION;
		// list = GTK_STRING_LIST(gtk_drop_down_get_model(dropdown));
        guint n_items = g_list_model_get_n_items(G_LIST_MODEL(list));
        
        // 在新列表中查找匹配的项
        for (guint i = 0; i < n_items; i++) {
            const char *text = gtk_string_list_get_string(list, i);
            if (g_strcmp0(text, selected_text) == 0) {
                new_pos = i;
                break;
            }
        }
        
        // 如果找到就选中，否则取消选中
        gtk_drop_down_set_selected(dropdown, new_pos);
        g_free((gpointer)selected_text);
    }

    // 返回 TRUE 让定时器继续运行
    return TRUE;
}

// struct open_close_comm_struct
// {
// 	GtkDropDown *dropdown;
// 	GtkButton *button;
// };


typedef struct listen_serial
{
	GThread *thread;
	gboolean keep_listen;
	gboolean activate;
	GSourceFunc     on_activate_func;
	gpointer        on_activate_data;
	GSourceFunc		on_disconnect_func;
	gpointer		on_disconnect_data;
}ListenSerialTypedef;

struct sp_port *selected_port = NULL;
static ListenSerialTypedef *listen1;
static GAsyncQueue *recv_queue = NULL;
static gint is_update_fpga = FALSE;

// 工作线程函数
gpointer listen_serial_thread(gpointer user_data) {
	// ListenSerialTypedef *listen1 = (ListenSerialTypedef *)user_data;
	while(listen1->keep_listen)
	{
		while(listen1->activate)
		{
			g_usleep(1000);
		}

		gint bytes_read = sp_input_waiting(selected_port);
		if (bytes_read == 0)
		{
			g_usleep(1000);
		}
		else if (bytes_read > 0)
		{
			listen1->activate = TRUE;
			g_idle_add(listen1->on_activate_func, listen1->on_activate_data);
		}
		else {
			g_print("Check Comm Input Buffer Fail! :%d\n", bytes_read);
			g_idle_add(listen1->on_disconnect_func, listen1->on_disconnect_data);
			return NULL;
		}
	}
    return NULL;
}

static gboolean on_serial_data(gpointer user_data) {
	if(user_data == NULL)
	{
		if(listen1 != NULL)
		{
			listen1->activate = FALSE;
		}
		return G_SOURCE_REMOVE;
	}

	// ListenSerialTypedef *listen1 = (ListenSerialTypedef *)user_data;
    GtkTextView *text_view = GTK_TEXT_VIEW(user_data);
    // 1. 检查错误条件
	g_print("entered: %d\n", 0);

    // 2. 读取数据 (cond 包含 G_IO_IN)
    unsigned char buffer[1024];
    gint bytes_read = 0;
    // GError *error = NULL;
    
    bytes_read = sp_nonblocking_read(selected_port, buffer, sizeof(buffer));
    // GIOStatus status = g_io_channel_read_chars(src, (gchar*)buffer, 
    //                                             sizeof(buffer), 
    //                                             &bytes_read, &error);
	
	if(bytes_read < 0)
	{
		if(listen1->keep_listen)
		{
			listen1->on_disconnect_func(listen1->on_disconnect_data);
		}
		return G_SOURCE_REMOVE;
	}

	if(g_atomic_int_get(&is_update_fpga))
	{
		// 1. 封装数据块（必须复制，因为 buffer 在栈上）
		SerialChunk *chunk = g_new(SerialChunk, 1);
		chunk->data = (uint8_t *)g_memdup2(buffer, bytes_read);
		chunk->length = bytes_read;
		
		// 2. 推入队列（GAsyncQueue 自带锁，线程安全）
		g_async_queue_push(recv_queue, chunk);
	}
	else 
	{
		// 处理接收到的数据 (注意：运行在主线程，可安全更新UI)
		// 写入 buffer
		GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(text_view);
		GtkTextIter end;
		gtk_text_buffer_get_end_iter(text_buffer, &end);
		GString *str = g_string_new("RECV: \n");
		for(guint i = 0; i < bytes_read; i++)
		{
			g_string_append_printf(str, "%x ", buffer[i]);
		}
		g_string_append_printf(str, "\n");
		// 使用 str->str 获取字符串
		gtk_text_buffer_insert(text_buffer, &end, str->str, -1);
		g_string_free(str, TRUE); // 释放
		
		// 自动滚动到底部
		gtk_text_buffer_get_end_iter(text_buffer, &end);
		gtk_text_view_scroll_to_iter(text_view, &end, 0.0, TRUE, 0.0, 0.0);
		// process_received_data(buffer, bytes_read);
	}

	listen1->activate = FALSE;
    return G_SOURCE_REMOVE;
}

static gboolean on_disconnect_process(gpointer user_data) {
    GtkButton *button_open_close = GTK_BUTTON(user_data);
	// 1. 移除事件监听
	if (listen1 != NULL) {
		listen1->keep_listen = FALSE;
		listen1->activate = FALSE;
		g_thread_join(listen1->thread);
		// // GTK4 中检查是否有待处理事件
		// while (g_main_context_pending(NULL)) {
		// 	g_main_context_iteration(NULL, FALSE);
		// }
		delete listen1;
	}
	sp_return error = sp_close(selected_port);
	sp_free_port(selected_port);
	selected_port = NULL;
	is_keep_scan = TRUE;
	gtk_button_set_label(button_open_close, "打开串口");
	GtkDropDown *dropdown = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button_open_close), "relate_dropdown"));
	gtk_widget_set_sensitive(GTK_WIDGET(dropdown), TRUE);
	GtkButton *button_send = GTK_BUTTON(g_object_get_data(G_OBJECT(button_open_close), "relate_button_send"));
	gtk_widget_set_sensitive(GTK_WIDGET(button_send), FALSE);
	log_printf(lm, "Comm Disconnect: %d", error);

    return G_SOURCE_REMOVE;
}


// 工作线程函数
gpointer send_data_thread(gpointer user_data) {
    BufferTypedef *buffer = (BufferTypedef *)user_data;

	// g_print("SEND_THREAD:%d\n", 0);
    
    // 在工作线程中，可以安全地使用阻塞写入
    int bytes_written = sp_blocking_write(selected_port, buffer->buf, buffer->len, 5000);
    
    if (bytes_written == buffer->len) {
        g_print("数据全部发送完成\n");
        // 通过 g_idle_add 将结果传回主线程更新 UI
        // g_idle_add(update_ui_on_success, NULL);
    } else if (bytes_written < 0) {
        // 发送失败
        char *err = sp_last_error_message();
        g_printerr("发送失败: %s\n", err);
        sp_free_error_message(err);
        // g_idle_add(update_ui_on_error, g_strdup(err));
    } else {
        // 部分发送（通常因为超时）
        g_print("仅发送了 %d / %zd 字节\n", bytes_written, buffer->len);
    }
    
    return NULL;
}


static GtkWidget *file_progress_bar = NULL;

// 回调函数，更新进度条
gboolean update_progress(gpointer user_data)
{
    // GtkWidget *progress_bar = GTK_WIDGET(user_data);
	if(file_progress_bar == NULL)
	{
		log_printf(lm, "Progress Bar Is NULL!");
		return FALSE;
	}

	if(user_data == NULL)
	{
		log_printf(lm, "Fraction Value Is NULL!");
		return FALSE;
	}

    float fraction = *(float*)user_data;
	g_free(user_data);

    // 在百分比模式下更新进度
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(file_progress_bar), fraction);
    
    // 同时更新显示文字
    char *text = g_strdup_printf("%.0f%% 完成", fraction * 100);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(file_progress_bar), text);
    g_free(text);

    // 返回 TRUE 让定时器继续运行
    return FALSE;
}

gint is_flash_read = FALSE;
gboolean in_send_file = FALSE;
GtkButton *send_button = NULL;

gboolean set_send_button_sensetive_true(gpointer user_data)
{
	GtkButton *button = GTK_BUTTON(user_data);
	gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
	return FALSE;
}

// 工作线程函数
gpointer send_file_thread(gpointer user_data) {
	char *path = (char *)user_data;
	int len = strlen(path);
	if(len <= 4)
	{
		log_printf(lm, "File Path Too Short!");
		g_free(path);
		in_send_file = FALSE;
		g_idle_add(set_send_button_sensetive_true, send_button);
		return NULL;
	}
	char *suffix = path + len - 4;

	if(g_atomic_int_get(&is_update_fpga))
	{
		gint8 error = enter_flash_mode();

		if(error != 0)
		{
			log_printf(lm, "Enter Flash Mode Fail! : %d", error);
		}
		else 
		{
			if(g_atomic_int_get(&is_flash_read))
			{
				gint8 error = flash_readback(update_progress);

				if(error != 0)
				{
					log_printf(lm, "Readback FPGA Program Error! : %d", error);
				}
				else 
				{
					log_printf(lm, "Readback FPGA Program Success!");
				}
			}
			else 
			{
				if(strncmp(suffix, ".bin", 4) != 0)
				{
					log_printf(lm, "File Name Format Error!");
					g_free(path);
					in_send_file = FALSE;
					g_idle_add(set_send_button_sensetive_true, send_button);
					return NULL;
				}

				gint8 error = flash_update_fpga(path, update_progress);

				if(error != 0)
				{
					log_printf(lm, "Update FPGA Program Error! : %d", error);
				}
				else 
				{
					log_printf(lm, "Update FPGA Program Success!");
				}
			}
		}
	}
	else 
	{
		if(strncmp(suffix, ".wsx", 4) != 0)
		{
			log_printf(lm, "File Name Format Error!");
			g_free(path);
			in_send_file = FALSE;
			g_idle_add(set_send_button_sensetive_true, send_button);
			return NULL;
		}

		gint8 error = parse_wsx(path, update_progress);

		if(error != 0)
		{
			log_printf(lm, "Send File Error! : %d", error);
		}
		else 
		{
			log_printf(lm, "Send File Success!");
		}
	}

	g_free(path);
	in_send_file = FALSE;
	g_idle_add(set_send_button_sensetive_true, send_button);
	return NULL;
}


gboolean open_close_comm(GtkButton *button, gpointer user_data)
{
    GtkDropDown *dropdown = GTK_DROP_DOWN(user_data);
	GtkSortListModel *model = GTK_SORT_LIST_MODEL(gtk_drop_down_get_model(dropdown));
	GtkStringList *list = GTK_STRING_LIST(gtk_sort_list_model_get_model(model));

	guint selected_pos = gtk_drop_down_get_selected(dropdown);
	if (selected_pos == GTK_INVALID_LIST_POSITION) {
		log_printf(lm, "Comm Unselected!");
		return -1;
	}
    const char *selected_text = gtk_string_list_get_string(list, selected_pos);

	struct sp_port **ports;
	// 调用 sp_list_ports 获取端口列表
	sp_return error = sp_list_ports(&ports);
	if (error != SP_OK)
	{
		log_printf(lm, "Scan Comm Fail! :%d", error);
		return -1;
	}
	guint i;
	for (i = 0; (ports[i] != NULL); i++) {
		// 打印每个端口的名称
		char *s = sp_get_port_description(ports[i]);
		if(s == NULL)
		{
			// sp_return error = sp_open(ports[i], SP_MODE_READ);
			// if(error == SP_OK)
			// {
			// 	continue;
			// }
			// log_printf(lm, "Open NULL Description Comm Fail! :%d", error);
			// std::string str = gbk_to_utf8(sp_last_error_message());
			// log_printf(lm, str.c_str());
			// return -1;
			continue;
		}
		std::string str = gbk_to_utf8(s);
		if(g_strcmp0(str.c_str(), selected_text) == 0)
		{
			break;
		}
	}
	if(ports[i])
	{
		if(g_strcmp0(gtk_button_get_label(button), "打开串口") == 0)
		{
			is_keep_scan = FALSE;
			if(selected_port != NULL)
			{
				sp_close(selected_port);
				sp_free_port(selected_port);
				selected_port = NULL;
			}
			sp_copy_port(ports[i], &selected_port);

			sp_return error = sp_open(selected_port, SP_MODE_READ_WRITE);
			if(error != SP_OK)
			{
				log_printf(lm, "Comm Open Fail! :%d", error);
				std::string str = gbk_to_utf8(sp_last_error_message());
				log_printf(lm, str.c_str());
				is_keep_scan = TRUE;
				return -1;
			}

			GtkTextView *text_view = GTK_TEXT_VIEW(g_object_get_data(G_OBJECT(button), "relate_text_view"));

			listen1 = new ListenSerialTypedef();
			listen1->keep_listen = TRUE;
			listen1->activate = FALSE;
			listen1->on_activate_func = on_serial_data;
			listen1->on_activate_data = text_view;
			listen1->on_disconnect_func = on_disconnect_process;
			listen1->on_disconnect_data = button;
			GThread *thread = g_thread_new("receive-thread", listen_serial_thread, NULL);
			listen1->thread = thread;

			gtk_button_set_label(button, "关闭串口");
			gtk_widget_set_sensitive(GTK_WIDGET(dropdown), FALSE);
			GtkButton *button_send = GTK_BUTTON(g_object_get_data(G_OBJECT(button), "relate_button_send"));
			gtk_widget_set_sensitive(GTK_WIDGET(button_send), TRUE);
			log_printf(lm, "Comm Open Success. ");
		}
		else 
		{
			// 1. 移除事件监听
			if (listen1 != NULL) {
				listen1->keep_listen = FALSE;
				g_thread_join(listen1->thread);
				// GTK4 中检查是否有待处理事件
				while (g_main_context_pending(NULL)) {
					g_main_context_iteration(NULL, FALSE);
				}
				delete listen1;
			}
			sp_return error = sp_close(selected_port);
			if(error != SP_OK)
			{
				log_printf(lm, "Comm Close Fail! :%d", error);
				return -1;
			}
			sp_free_port(selected_port);
			selected_port = NULL;
			is_keep_scan = TRUE;
			gtk_button_set_label(button, "打开串口");
			gtk_widget_set_sensitive(GTK_WIDGET(dropdown), TRUE);
			GtkButton *button_send = GTK_BUTTON(g_object_get_data(G_OBJECT(button), "relate_button_send"));
			gtk_widget_set_sensitive(GTK_WIDGET(button_send), FALSE);
			log_printf(lm, "Comm Close Success. ");
		}
	}
	else 
	{
		log_printf(lm, "Comm Not Find!");

		is_keep_scan = TRUE;
		gtk_button_set_label(button, "打开串口");
		gtk_widget_set_sensitive(GTK_WIDGET(dropdown), TRUE);
		GtkButton *button_send = GTK_BUTTON(g_object_get_data(G_OBJECT(button), "relate_button_send"));
		gtk_widget_set_sensitive(GTK_WIDGET(button_send), FALSE);

		return -1;
	}
	// 释放端口列表内存
	sp_free_port_list(ports);
	return 0;
}

// #define TEST_LEN	32
// uint8_t data[TEST_LEN];
// BufferTypedef buffer = {&data[0], TEST_LEN};

gboolean send_file(GtkButton *button, gpointer user_data)
{
	GtkEntry *entry = GTK_ENTRY(user_data);
	GtkCheckButton *btn1 = GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "relate_checkbutton_setwave"));
	GtkCheckButton *btn2 = GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "relate_checkbutton_updatefpga"));

	char *path = g_strdup(gtk_editable_get_text(GTK_EDITABLE(entry)));

	if(in_send_file)
	{
		log_printf(lm, "In Send File, Busy");
		return FALSE;
	}
	in_send_file = TRUE;
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);


	if(gtk_check_button_get_active(GTK_CHECK_BUTTON(btn1)))
	{
		g_atomic_int_set(&is_update_fpga, FALSE);
		g_atomic_int_set(&is_flash_read, FALSE);
	}
	else if(gtk_check_button_get_active(GTK_CHECK_BUTTON(btn2)))
	{
		g_atomic_int_set(&is_update_fpga, TRUE);
		g_atomic_int_set(&is_flash_read, FALSE);
	}
	else 
	{
		g_atomic_int_set(&is_update_fpga, TRUE);
		g_atomic_int_set(&is_flash_read, TRUE);
	}


    GThread *thread = g_thread_new("send-file", send_file_thread, path);
	g_thread_unref(thread);

	//Get File Path
	//Read File
	//Send File

	// for(guint i = 0; i < TEST_LEN; i++)
	// {
	// 	data[i] = i;
	// }
	
    // if (selected_port == NULL) {
    //     g_printerr("串口未打开，无法发送数据\n");
    //     return TRUE;
    // }

    // 使用非阻塞写入，避免阻塞 GTK 主循环
    // int bytes_written = sp_nonblocking_write(selected_port, data, TEST_LEN);
    // int bytes_written = sp_blocking_write(selected_port, data, TEST_LEN, 500);
    // GThread *thread = g_thread_new("send-thread", send_data_thread, &buffer);
	// g_thread_unref(thread);

    // if (bytes_written < 0) {
    //     char *err = sp_last_error_message();
    //     g_printerr("发送数据失败: %s\n", err);
    //     sp_free_error_message(err);
    //     return FALSE;
    // } else if (bytes_written < TEST_LEN) {
    //     // 非阻塞模式下，可能只写入部分数据
    //     g_warning("仅发送了 %d / %d 字节\n", bytes_written, TEST_LEN);
    // }


	return FALSE;
}

static gboolean is_readback_on;

static void on_activate(GtkApplication *app) {
    // 创建一个新窗口
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "USB Wave Generater");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);


    GtkWidget *box_vert = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	// 设置盒子四周的边距为 20 像素
	gtk_widget_set_margin_start (GTK_WIDGET(box_vert), 5);
	gtk_widget_set_margin_end   (GTK_WIDGET(box_vert), 5);
	gtk_widget_set_margin_top   (GTK_WIDGET(box_vert), 5);
	gtk_widget_set_margin_bottom(GTK_WIDGET(box_vert), 5);
	gtk_window_set_child (GTK_WINDOW (window), box_vert);
    GtkWidget *box_hori0 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *box_hori1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box_vert), box_hori0);
    gtk_box_append(GTK_BOX(box_vert), box_hori1);
	// GtkWidget *grid = gtk_grid_new();
	// gtk_window_set_child (GTK_WINDOW (window), grid);

	
    GtkWidget *label1 = gtk_label_new("文件");
    gtk_box_append(GTK_BOX(box_hori0), label1);

    GtkWidget *entry = gtk_entry_new();
    // 创建一个 "search" 主题图标
    GIcon *search_icon = g_themed_icon_new("document-open");
    // 将图标设置在输入框的次要位置（通常为右侧）
    gtk_entry_set_icon_from_gicon(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, search_icon);
	// gtk_entry_set_icon_activatable(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, TRUE);
    // 3. 连接 icon-press 信号
    g_signal_connect(entry, "icon-press", G_CALLBACK(on_icon_press), NULL);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(box_hori0), entry);

	// GtkWidget *button1 = gtk_button_new_with_label ("选择文件");
	// g_signal_connect (button1, "clicked", G_CALLBACK (print_hello), NULL);
	// gtk_grid_attach (GTK_GRID (grid), button1, 4, 0, 1, 1);
	

    // 创建一个标签，用于显示文字
    // 将标签放入窗口
    GtkWidget *label2 = gtk_label_new("端口号");
    gtk_box_append(GTK_BOX(box_hori1), label2);
	
	GtkStringList *string_list = gtk_string_list_new(NULL);
    struct sp_port **ports;
    // 调用 sp_list_ports 获取端口列表
    sp_return error = sp_list_ports(&ports);
    if (error == SP_OK) {
        for (int i = 0; ports[i]; i++) {
            // 打印每个端口的名称
			char *s = sp_get_port_description(ports[i]);
			if(s == NULL)
			{
				continue;
			}
			std::string str = gbk_to_utf8(s);
			gtk_string_list_append(string_list, str.c_str());
        }
        // 释放端口列表内存
        sp_free_port_list(ports);
    }
	// 创建排序模型
	GtkSortListModel *sort_model = gtk_sort_list_model_new(
		G_LIST_MODEL(string_list),
		NULL  // 先用 NULL，后面设置排序器
	);

	// 创建字符串排序器（按字母升序）
	GtkStringSorter *sorter = gtk_string_sorter_new(
		gtk_property_expression_new(GTK_TYPE_STRING_OBJECT, NULL, "string")
	);
	gtk_sort_list_model_set_sorter(sort_model, GTK_SORTER(sorter));

	// 把排序模型给 DropDown
	GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(sort_model), NULL);
	// GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(string_list), NULL);
    // 连接信号，监听 popup 属性的变化
    gtk_box_append(GTK_BOX(box_hori1), dropdown);
    g_timeout_add(500, update_dropdown, dropdown);

	GtkWidget *button2 = gtk_button_new_with_label ("打开串口");
	g_signal_connect (button2, "clicked", G_CALLBACK (open_close_comm), dropdown);
    gtk_box_append(GTK_BOX(box_hori1), button2);

    GtkWidget *label_blank = gtk_label_new(" ");
    gtk_widget_set_hexpand(label_blank, TRUE);
    gtk_box_append(GTK_BOX(box_hori1), label_blank);

	GtkWidget *btn1 = gtk_check_button_new_with_label("设置波形");
	GtkWidget *btn2 = gtk_check_button_new_with_label("更新FPGA");
	// 编组
	gtk_check_button_set_group(GTK_CHECK_BUTTON(btn2), GTK_CHECK_BUTTON(btn1));
	// g_signal_connect(btn1, "toggled", G_CALLBACK(on_check_toggled), user_data);
	if(is_readback_on)
	{
		GtkWidget *btn3 = gtk_check_button_new_with_label("读取FPGA程序");
		gtk_check_button_set_group(GTK_CHECK_BUTTON(btn3), GTK_CHECK_BUTTON(btn1));
		gtk_box_append(GTK_BOX(box_hori1), btn3);
	}
    gtk_box_append(GTK_BOX(box_hori1), btn2);
    gtk_box_append(GTK_BOX(box_hori1), btn1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn1), TRUE);

	GtkWidget *button3 = gtk_button_new_with_label ("发送");
	g_signal_connect (button3, "clicked", G_CALLBACK (send_file), entry);
	send_button = GTK_BUTTON(button3);
    gtk_box_append(GTK_BOX(box_hori1), button3);
	gtk_widget_set_sensitive(GTK_WIDGET(button3), FALSE);
	g_object_set_data(G_OBJECT(button3), "relate_checkbutton_setwave", btn1);
	g_object_set_data(G_OBJECT(button3), "relate_checkbutton_updatefpga", btn2);

	g_object_set_data(G_OBJECT(button2), "relate_button_send", button3);
	g_object_set_data(G_OBJECT(button2), "relate_dropdown", dropdown);


    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_box_append(GTK_BOX(box_vert), progress_bar);
	file_progress_bar = progress_bar;
    // 添加一个定时器，每 200 毫秒调用一次 update_progress
    // g_timeout_add(200, update_progress, progress_bar);


    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,  // 水平滚动：按需
                                   GTK_POLICY_AUTOMATIC); // 垂直滚动：按需
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(box_vert), scrolled);

    GtkWidget *text_view = gtk_text_view_new();
    // 设置为不可编辑
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
	g_object_set_data(G_OBJECT(button2), "relate_text_view", text_view);
    
    // 设置文本内容
    // GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    // gtk_text_buffer_set_text(buffer, "This text cannot be edited", -1);
    lm = log_manager_new(GTK_TEXT_VIEW(text_view));
	log_printf(lm, "USB Wave Generater. ");
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), text_view);
    

	recv_queue = g_async_queue_new();
	extern gint recv_packet_thread_is_run;
	g_atomic_int_set(&recv_packet_thread_is_run, TRUE);
    GThread *thread_recv_packet = g_thread_new("recv_packet", recv_packet, recv_queue);
	g_thread_unref(thread_recv_packet);
	
	wave_generater_init();

	// list_ports(&tmp_s[0]);
    // GtkWidget *label = gtk_label_new(&tmp_s[0]);

	

	// GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	// gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
	// gtk_widget_set_valign (box, GTK_ALIGN_CENTER);

	// gtk_window_set_child (GTK_WINDOW (window), box);
	// // g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), window);

	// gtk_box_append (GTK_BOX (box), button);

    // 显示窗口
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
	
	is_readback_on = FALSE;
	
	if((argc == 2) && (atoi(argv[1]) == 1))
	{
		is_readback_on = TRUE;
		argc = 1;
	}

    // 创建GTK应用对象
    GtkApplication *app = gtk_application_new("com.example.USBWaveGenerater", G_APPLICATION_DEFAULT_FLAGS);
    
    // 连接"activate"信号，当应用启动时调用on_activate函数
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    
    // 运行应用，传入命令行参数
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // 释放应用对象
    g_object_unref(app);
    
    return status;
}