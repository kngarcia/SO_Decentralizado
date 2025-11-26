/* kernel/ipc/message.c - simple in-kernel message queue (very small) */
#include <stdint.h>

#define MSG_MAX 32
#define MSG_SIZE 128

static char msgs[MSG_MAX][MSG_SIZE];
static int head = 0, tail = 0, count = 0;

int ipc_send(const char *s) {
    if (count >= MSG_MAX) return -1;
    int idx = tail++;
    if (tail >= MSG_MAX) tail = 0;
    for (int i=0;i<MSG_SIZE-1 && s[i];i++) msgs[idx][i]=s[i];
    msgs[idx][MSG_SIZE-1]=0;
    count++;
    return 0;
}

int ipc_recv(char *buf, int buflen) {
    if (count == 0) return -1;
    int idx = head++;
    if (head >= MSG_MAX) head=0;
    int i;
    for (i=0; i<buflen-1 && msgs[idx][i]; i++) buf[i]=msgs[idx][i];
    buf[i]=0;
    count--;
    return 0;
}
