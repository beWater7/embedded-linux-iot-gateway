#ifndef __LOGMNG_H__
#define __LOGMNG_H__

#include "list.h"
#include <pthread.h>
#include <mqueue.h> // posix消息队列
#include <time.h>

#define NAME_MAX_LEN 32     
#define LOG_ITEM_NUM 10  //日志条目大小
#define LOG_ITEM_LEN 128  //一条日志最大长度
#define LOG_FILE_MAX_NUM  20 //日志文件最大数量


/* 定义日志类型 */
#define MAJORTYPE 0x0001
#define OPERATION 0x0002
#define EVENT     0x0003
#define EXCEPTION 0x0004


#define MINORTYPE 0x0020

//#define LOG_FOLDER "/home/liudayi/code/learn-test/LogFolder"
#define LOG_FOLDER "/tmp/LogFolder"
/* 会自动生成在/dev/mqueue/ 路径下
 * queue的取名非常重要, 取名log_queue时一直无法设置属性，队列一直是满的
 */
#define LOG_QUEUE "/tmp/my_queue" 

#define TIME_TYPE time_t

typedef struct log_sew{
    TIME_TYPE startTime; // 日志条目最早时间
    TIME_TYPE endtime;   // 日志条目最晚时间
}LOG_SEW;


typedef struct log_write_param{
    unsigned short major;
    unsigned short minor;
    TIME_TYPE time;
}LOG_WRITE_PARAM;


typedef struct log_read_param{
    unsigned short major;
    unsigned short minor;
    //unsigned long time;
    TIME_TYPE time;
}LOG_READ_PARAM;


typedef struct log_node{
    NODE node;
    char fileName[NAME_MAX_LEN];
}LOG_NODE;


typedef struct log_item{
    unsigned long magic;
    unsigned short major;
    unsigned short minor;
    int seq;
    TIME_TYPE time;         // 记录时间
    //user
    //ipAddr
    unsigned int info_len;      //日志长度
    char info[LOG_ITEM_LEN-28]; //日志信息:100字节
}LOG_ITEM; /*日志条目结构体*/


typedef struct {
    long mtype;
    LOG_ITEM data;
}MSG_T;


typedef struct log_item_node{
    NODE node;
    LOG_ITEM log_item;
}LOG_ITEM_NODE;


/* 保存在flash中的日志文件头信息
 * 
 */
typedef struct logHeadInfo{
    TIME_TYPE startTime;     // 最早时间
    TIME_TYPE endTime;       // 最晚时间
    int startSeq;                // 第一条日志序号
    int endSeq;                  // 最后一条日志序号      
    unsigned int fileWriteSeq;   // 文件可写位置
    unsigned int fileReadSeq;    // 文件可读位置
    unsigned char logItemNum;    // 文件中日志条目的数量
    char res1[3];              // 保留字段1
    LOG_SEW log_set[LOG_ITEM_NUM];       // 日志时间数组
    //checksum
    char res2[12];               // 保留字段
}LOG_HEAD_INFO; /*文件头结构体*/


/* 保存在内存中的文件信息结构体
 * 考虑字节对齐 4字节
 */
typedef struct logFileInfo{
    char fileName[NAME_MAX_LEN]; // 保存文件名
    int seq;                     // 文件序号
    int logFilefd;               // 文件的fd描述符
    unsigned char logItemNum;    // 文件中日志条目的数量
    LOG_HEAD_INFO logHead;       // 日志头
    char reserv[12];             // 保留字段
}LOG_FILE_INFO; /*文件头结构体*/



typedef struct logMng{
    LIST logFileList;          // 日志文件管理链表
    LOG_FILE_INFO logFileInfo; // 当前日志文件
    int logFileNum;            // 当前日志文件数量
    int  MsgQid;                // 消息队列的Qid
    pthread_mutex_t logMngMutex;     // 互斥锁
    TIME_TYPE logLimitTime;
}LOG_MNG_T, *LOG_MNG_PTR;

//int initLogMng(void);
//int writeLog(LOG_WRITE_PARAM *log_write_param, char *data, int dateLen);
void startLogMngTask(void);
int writeLogApi(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen);
#endif


