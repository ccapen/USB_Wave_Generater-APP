#ifndef _TEXT_VIEW_LOG_
#define _TEXT_VIEW_LOG_


#include <gtk/gtk.h>
#include <stdarg.h>
#include <time.h>


// 日志管理结构
typedef struct {
    GtkTextBuffer *buffer;
    GtkTextView *text_view;
    gboolean auto_scroll;
    gboolean show_timestamp;
} LogManager;


LogManager* log_manager_new(GtkTextView *text_view);
void log_printf(LogManager *lm, const char *format, ...);
void log_printf_colored(LogManager *lm, const char *color, const char *format, ...);


#endif
