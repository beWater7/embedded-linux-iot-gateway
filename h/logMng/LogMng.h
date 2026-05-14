#ifndef __LOGMNG_H__
#define __LOGMNG_H__

#include "list.h"
#include <pthread.h>
#include <mqueue.h>
#include <time.h>

#define NAME_MAX_LEN            32
#define LOG_ITEM_NUM            10      /* 文件头中记录的时间段数量 */
#define LOG_ITEM_LEN            128     /* LOG_ITEM 结构体总字节数  */
#define LOG_INFO_LEN            (LOG_ITEM_LEN - 28) /* info 字段实际可用字节数(100B) */
#define LOG_FILE_MAX_NUM        20      /* 日志文件最大数量（超限自动轮转删旧） */
#define MAX_LOG_ITEMS_PER_FILE  10      /* 每个日志文件最多存储的条目数 */

#ifndef PATH_MAX
#define PATH_MAX  1024
#endif

/* 日志类型（major） */
#define MAJORTYPE   0x0001
#define OPERATION   0x0002
#define EVENT       0x0003
#define EXCEPTION   0x0004
#define MINORTYPE   0x0020

/* 日志存储目录与消息队列名 */
#define LOG_FOLDER  "/home/root/LogFolder"
#define LOG_QUEUE   "/tmp/my_queue"

#define TIME_TYPE time_t

typedef struct log_sew {
    TIME_TYPE startTime;
    TIME_TYPE endtime;
} LOG_SEW;

typedef struct log_write_param {
    unsigned short major;
    unsigned short minor;
    TIME_TYPE      time;
} LOG_WRITE_PARAM;

typedef struct log_read_param {
    unsigned short major;
    unsigned short minor;
    TIME_TYPE      time;
} LOG_READ_PARAM;

typedef struct log_node {
    NODE node;
    char fileName[NAME_MAX_LEN];
} LOG_NODE;

/*
 * 日志条目结构体，总大小固定为 LOG_ITEM_LEN(128) 字节。
 * info 字段大小由 LOG_INFO_LEN 统一管理，避免硬编码魔数。
 *
 * 字段布局(字节):
 *   magic(8) + major(2) + minor(2) + seq(4) + time(8) + info_len(4) = 28B
 *   info = 128 - 28 = 100B
 */
typedef struct log_item {
    unsigned long  magic;
    unsigned short major;
    unsigned short minor;
    int            seq;
    TIME_TYPE      time;
    unsigned int   info_len;
    char           info[LOG_INFO_LEN];
} LOG_ITEM;

typedef struct {
    long     mtype;
    LOG_ITEM data;
} MSG_T;

typedef struct log_item_node {
    NODE     node;
    LOG_ITEM log_item;
} LOG_ITEM_NODE;

/* 保存在 flash/文件中的文件头信息 */
typedef struct logHeadInfo {
    TIME_TYPE     startTime;
    TIME_TYPE     endTime;
    int           startSeq;
    int           endSeq;
    unsigned int  fileWriteSeq;  /* 下次写入位置（文件内偏移） */
    unsigned int  fileReadSeq;
    unsigned char logItemNum;    /* 已写入的条目数 */
    char          res1[3];
    LOG_SEW       log_set[LOG_ITEM_NUM];
    char          res2[12];
} LOG_HEAD_INFO;

/* 保存在内存中的当前文件信息 */
typedef struct logFileInfo {
    char          fileName[NAME_MAX_LEN];
    int           seq;
    int           logFilefd;
    unsigned char logItemNum;
    LOG_HEAD_INFO logHead;
    char          reserv[12];
} LOG_FILE_INFO;

typedef struct logMng {
    LIST            logFileList;
    LOG_FILE_INFO   logFileInfo;
    int             logFileNum;
    int             MsgQid;
    pthread_mutex_t logMngMutex;
    TIME_TYPE       logLimitTime;
    int             init_result;  /* 0: 初始化成功  -1: 初始化失败 */
} LOG_MNG_T, *LOG_MNG_PTR;

void startLogMngTask(void);
int  writeLogApi(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen);

#endif
