#include "text_view_log.hpp"

// 初始化日志管理器
LogManager* log_manager_new(GtkTextView *text_view) {
    LogManager *lm = g_new(LogManager, 1);
    lm->text_view = text_view;
    lm->buffer = gtk_text_view_get_buffer(text_view);
    lm->auto_scroll = TRUE;
    lm->show_timestamp = TRUE;
    return lm;
}

struct lm_msg_struct
{
	LogManager *lm;
	GString *msg;
};

gboolean add_to_text_view(gpointer user_data)
{
	struct lm_msg_struct *lg = (struct lm_msg_struct *)user_data;
	LogManager *lm = lg->lm;
	GString *msg = lg->msg;
    // 写入 buffer
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(lm->buffer, &end);
    gtk_text_buffer_insert(lm->buffer, &end, msg->str, -1);
    
    // 自动滚动到底部
    if (lm->auto_scroll && lm->text_view) {
        gtk_text_buffer_get_end_iter(lm->buffer, &end);
        gtk_text_view_scroll_to_iter(lm->text_view, &end, 0.0, TRUE, 0.0, 0.0);
    }
    
    g_string_free(msg, TRUE);
	g_free(lg);

	return FALSE;
}

// 核心输出函数
void log_printf(LogManager *lm, const char *format, ...) {
    if (!lm || !lm->buffer) return;
    
    va_list args;
    va_start(args, format);
    
    // 构建消息
    GString *msg = g_string_new(NULL);
    
    // 添加时间戳
    if (lm->show_timestamp) {
        GDateTime *now = g_date_time_new_now_local();
        char *time_str = g_date_time_format(now, "[%H:%M:%S] ");
        g_string_append(msg, time_str);
        g_free(time_str);
        g_date_time_unref(now);
    }
    
    // 格式化用户消息
    char *user_msg = g_strdup_vprintf(format, args);
    g_string_append(msg, user_msg);
    g_string_append(msg, "\n");
    g_free(user_msg);
    
    va_end(args);
    
	struct lm_msg_struct *lg = new struct lm_msg_struct();
	lg->lm = lm;
	lg->msg = msg;
	g_idle_add(add_to_text_view, lg);
}

// 颜色输出（使用 Pango 标记）
void log_printf_colored(LogManager *lm, const char *color, const char *format, ...) {
    if (!lm || !lm->buffer) return;
    
    va_list args;
    va_start(args, format);
    
    char *user_msg = g_strdup_vprintf(format, args);
    va_end(args);
    
    // 构建带颜色的消息
    char *colored_msg = g_strdup_printf(
        "<span foreground=\"%s\">%s</span>\n", 
        color, user_msg
    );
    
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(lm->buffer, &end);
    gtk_text_buffer_insert_markup(lm->buffer, &end, colored_msg, -1);
    
    if (lm->auto_scroll && lm->text_view) {
        gtk_text_buffer_get_end_iter(lm->buffer, &end);
        gtk_text_view_scroll_to_iter(lm->text_view, &end, 0.0, TRUE, 0.0, 0.0);
    }
    
    g_free(user_msg);
    g_free(colored_msg);
}

// // 便捷宏
// #define LOG_INFO(lm, ...) log_printf(lm, __VA_ARGS__)
// #define LOG_ERROR(lm, ...) log_printf_colored(lm, "red", __VA_ARGS__)
// #define LOG_WARN(lm, ...) log_printf_colored(lm, "orange", __VA_ARGS__)
// #define LOG_SUCCESS(lm, ...) log_printf_colored(lm, "green", __VA_ARGS__)

// // 完整使用示例
// int main(int argc, char *argv[]) {
//     gtk_init(&argc, &argv);
    
//     GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//     gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    
//     GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
//     gtk_container_add(GTK_CONTAINER(window), vbox);
    
//     // 创建文本视图（用于日志显示）
//     GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
//     gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
//     GtkWidget *text_view = gtk_text_view_new();
//     gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
//     gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
//     gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
//     gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    
//     // 设置字体（可选）
//     PangoFontDescription *font = pango_font_description_from_string("Monospace 10");
//     gtk_widget_override_font(text_view, font);
//     pango_font_description_free(font);
    
//     // 启用 Pango 标记（用于颜色）
//     GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
//     gtk_text_buffer_create_tag(buffer, "red", "foreground", "red", NULL);
//     gtk_text_buffer_create_tag(buffer, "green", "foreground", "green", NULL);
//     gtk_text_buffer_create_tag(buffer, "orange", "foreground", "orange", NULL);
//     gtk_text_buffer_create_tag(buffer, "blue", "foreground", "blue", NULL);
    
//     // 创建日志管理器
//     LogManager *lm = log_manager_new(GTK_TEXT_VIEW(text_view));
    
//     // 测试输出
//     LOG_INFO(lm, "=== Application Started ===");
//     LOG_INFO(lm, "System: %s %s", "Linux", "6.0");
//     LOG_INFO(lm, "User: %s", g_get_user_name());
//     LOG_SUCCESS(lm, "Initialization complete");
//     LOG_WARN(lm, "Configuration file not found, using defaults");
//     LOG_ERROR(lm, "Failed to connect to server: %s", "Connection refused");
//     LOG_INFO(lm, "Numeric test: %d, %.2f, %ld", 42, 3.14159, time(NULL));
    
//     // 控制按钮
//     GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
//     gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    
//     GtkWidget *btn_info = gtk_button_new_with_label("Info");
//     GtkWidget *btn_warn = gtk_button_new_with_label("Warning");
//     GtkWidget *btn_error = gtk_button_new_with_label("Error");
//     GtkWidget *btn_clear = gtk_button_new_with_label("Clear");
    
//     gtk_box_pack_start(GTK_BOX(hbox), btn_info, TRUE, TRUE, 0);
//     gtk_box_pack_start(GTK_BOX(hbox), btn_warn, TRUE, TRUE, 0);
//     gtk_box_pack_start(GTK_BOX(hbox), btn_error, TRUE, TRUE, 0);
//     gtk_box_pack_start(GTK_BOX(hbox), btn_clear, TRUE, TRUE, 0);
    
//     // 按钮回调
//     g_signal_connect(btn_info, "clicked", G_CALLBACK([](GtkWidget *w, gpointer data) {
//         LogManager *lm = (LogManager*)data;
//         LOG_INFO(lm, "Info button clicked at %ld", time(NULL));
//     }), lm);
    
//     g_signal_connect(btn_warn, "clicked", G_CALLBACK([](GtkWidget *w, gpointer data) {
//         LogManager *lm = (LogManager*)data;
//         LOG_WARN(lm, "Warning: Button clicked but something might be wrong");
//     }), lm);
    
//     g_signal_connect(btn_error, "clicked", G_CALLBACK([](GtkWidget *w, gpointer data) {
//         LogManager *lm = (LogManager*)data;
//         LOG_ERROR(lm, "Critical error occurred!");
//     }), lm);
    
//     g_signal_connect(btn_clear, "clicked", G_CALLBACK([](GtkWidget *w, gpointer data) {
//         LogManager *lm = (LogManager*)data;
//         gtk_text_buffer_set_text(lm->buffer, "", -1);
//         LOG_INFO(lm, "=== Log Cleared ===");
//     }), lm);
    
//     g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
//     gtk_widget_show_all(window);
//     gtk_main();
    
//     g_free(lm);
//     return 0;
// }