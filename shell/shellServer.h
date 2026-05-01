#ifndef __SHELLSERVER_H__
#define __SHELLSERVER_H__

#include <pthread.h>
#include "../h/frameWork/list.h"
#define SHELLBUFFLEN 256
#define SOCKET_PATH "/tmp/mysocket"
//#define SOCKET_PATH "/home/liudayi/code/learn-test/mysocket"         
typedef void (*Callback)(void *udata, char *recvbuf, char *sendbuf);

struct cmdheader{
    char cmdname[16];
    int argc;    // 参数个数
    int argvlen; // 参数长度
    int length;  // 所有长度
};

typedef struct {
  NODE node;
  char cmdname[16];
  Callback fn; //
  void *udata;
}shellunit;

struct shellcmdList
{
    LIST list;
    pthread_mutex_t mutex;
    char recvbuf[SHELLBUFFLEN];
    int recvbufLen;
    char sendbuf[SHELLBUFFLEN];
    int sendbufLen;
};

#define DEBUG  0
#if DEBUG
    #define SHELLSERVER_PRINT(fmt, ...) \
        do { \
            printf("[SHELL] " fmt, ##__VA_ARGS__); \
        } while(0)
#else
    #define SHELLSERVER_PRINT(fmt, ...) \
        do {} while(0)
#endif

int regShellCmd(char *name, Callback fn, void *udata);
void *shellServerTask(void *arg);
//pthread_t startShellServerTask();
void startShellServerTask(void);



#endif
