#ifndef _RECV_PACKET_H_
#define _RECV_PACKET_H_

#include <gtk/gtk.h>


#define PACKET_BUFFER_SIZE	550


typedef struct {
    unsigned char *data;   // 数据内容（堆分配）
    gsize length;          // 数据长度
} SerialChunk;


gpointer recv_packet(gpointer user_data);


#endif
