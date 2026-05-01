#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>     // 用于遍历目录下所有文件的函数
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include "LogMng.h"

#define KEY 1234

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //互斥锁
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // 条件变量，用于通知主线程子线程已经初始化完成
static int thread_initialized = 0;  

LOG_MNG_T gs_log_mng_t;

/* 等待模块初始化完毕 */
static void waitModuleInited()
{
    pthread_mutex_lock(&mutex);
    while (!thread_initialized)
    {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}

// 比较函数，用于排序文件名
int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int initLogMng(void)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    struct stat info;
    char **files = NULL;
    int file_count = 0;
    int fd = -1;
    LOG_NODE *log_node = NULL;
    char tmpBuff[sizeof(LOG_FILE_INFO) + 1] = {0};
#if defined(USE_POSIX_MQ)
    mqd_t mq;
    struct mq_attr attr = {0};
#else
    //MSG_T msg = {0};
#endif

    /* 初始化日志管理结构体 */
    memset(&gs_log_mng_t, 0, sizeof(LOG_MNG_T));
    gs_log_mng_t.logFileInfo.logFilefd = -1;
    lstInit(&gs_log_mng_t.logFileList);
    if (pthread_mutex_init(&gs_log_mng_t.logMngMutex, NULL) != 0) 
    {
        perror("Mutex initialization failed");
        return -1;
    }
#if 1
    /* 创建用于通信的消息队列 (system V ipc)*/
    gs_log_mng_t.MsgQid = msgget(KEY, IPC_CREAT | 0666);
    if (-1 == gs_log_mng_t.MsgQid) 
    {
        perror("msgget");
        return -1;
    }
#endif
#if 1
#if defined(USE_POSIX_MQ)
    //printf("posix support!\n");
    /*  System V IPC msgget方式被认为是过时的，posix提供了更加强大、灵活的消息队列 */
    
    /* 设置队列属性 */
    attr.mq_msgsize = sizeof(LOG_ITEM) * 2;
    attr.mq_maxmsg = 20; // 例如，设置为允许10个消息在队列中
    attr.mq_curmsgs = 0;
    /* 将消息队列属性的非阻塞标志设置为 O_NONBLOCK
     * 属性只对当前进程有效
     */
    attr.mq_flags = O_NONBLOCK;
    /* 创建posix消息队列 */
    mq = mq_open(LOG_QUEUE, O_CREAT | O_RDWR, 0666, &attr);
    if ((mqd_t)-1 == mq) {
        perror("mq_open");
        exit(1);
    }

    // if (mq_setattr(mq, &attr, NULL) == -1) 
    // {
    //     perror("mq_setattr");
    //     exit(1);
    // }
    
    // 获取当前消息队列属性
    if (mq_getattr(mq, &attr) == -1) {
        perror("mq_getattr");
        exit(1);
    }
    //printf("mq = %d\n",mq);
    //printf("%d-%d-%d\n",attr.mq_maxmsg,attr.mq_msgsize,attr.mq_curmsgs);
#endif
#endif
    /* 遍历目录下所有文件，构建用于管理的日志文件链表 */

    if ((dir = opendir(LOG_FOLDER)) == NULL) 
    {
        if (errno == ENOENT)
        {
            // 目录不存在，创建
            if (mkdir(LOG_FOLDER, 0755) == -1)
            {
                perror("mkdir");
                return -1;
            }

            // 创建完再打开
            dir = opendir(LOG_FOLDER);
            if (dir == NULL)
            {
                perror("opendir after mkdir");
                return -1;
            }
        }
        else
        {
            // 其他错误，比如权限问题
            perror("opendir");
            return -1;
        }
    }

    while ((entry = readdir(dir)) != NULL) 
    {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", LOG_FOLDER, entry->d_name);

        if (stat(fullpath, &info) == -1) 
        {
            perror("stat");  // 打印出错信息
            continue;
        }

        if (S_ISDIR(info.st_mode)) 
        {
            // 目录
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
            {
                printf("Directory: %s\n", fullpath);
                //traverseDirectory(fullpath);  // 递归调用以遍历子目录 ---暂不支持
            }
        } 
        else if (S_ISREG(info.st_mode)) 
        {
            /* 普通日志文件, 遍历结果显示并不是理想中的顺序, 因此需要对文件进行排序 */
            //printf("File: %s\n", fullpath);
            files = realloc(files, (file_count + 1) * sizeof(char *));
            files[file_count] = strdup(entry->d_name);
            file_count++;          
        }
    }

    /* 排序文件名、标准库实现的快速排序算法 */
    qsort(files, file_count, sizeof(char *), compare);

    // 打印排序后的文件名
    for (int i = 0; i < file_count; i++) 
    {    
        log_node = (LOG_NODE*)malloc(sizeof(LOG_NODE));
        memcpy(log_node->fileName, files[i], NAME_MAX_LEN);
        pthread_mutex_lock(&gs_log_mng_t.logMngMutex);
        /* 将文件节点加入到管理链表 */
        lstAddTail(&gs_log_mng_t.logFileList, &log_node->node);
        gs_log_mng_t.logFileNum++;
        gs_log_mng_t.logFileInfo.seq++;
        pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
        printf("Sorted File: %s\n", files[i]);
        free(files[i]);  // 释放内存
    }

    /* 直接找到最后一个日志文件*/
    if(0 == lstLength(&gs_log_mng_t.logFileList))
    {
        printf("logFileList is NULL!\n");

        pthread_mutex_lock(&mutex);
        thread_initialized = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

        //close(gs_log_mng_t.logFileInfo.logFilefd);
        return 0;
    }
    
    log_node = (LOG_NODE *)lstLast(&gs_log_mng_t.logFileList);
    strncpy(gs_log_mng_t.logFileInfo.fileName, log_node->fileName, NAME_MAX_LEN);
    char dstFileName[128] = {0};
    /* 获取文件完整路径 */
    strncpy(dstFileName, LOG_FOLDER, strlen(LOG_FOLDER));
    dstFileName[strlen(LOG_FOLDER)] = '/';
    strcat(dstFileName,log_node->fileName);
    dstFileName[strlen(LOG_FOLDER) +  strlen(log_node->fileName) + 1] = '\0';
    //printf("lastestFileName = %s\n", dstFileName);

    fd = open(dstFileName, O_RDWR);
    if (-1 == fd)
    {
        perror("open");
        return -1;
    }
    /* 获取文件信息 */
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }

    pthread_mutex_lock(&gs_log_mng_t.logMngMutex);
    gs_log_mng_t.logFileInfo.logFilefd = fd;
    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;

    // 循环读取文件头信息
    while ((bytesRead = read(fd, tmpBuff + totalBytesRead, sizeof(LOG_HEAD_INFO) - totalBytesRead)) > 0) 
    {
        totalBytesRead += bytesRead;
    }

    if(totalBytesRead != sizeof(LOG_HEAD_INFO))
    {
        printf("read file head failed!\n");
        pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
        return -1;
    }
    tmpBuff[sizeof(LOG_HEAD_INFO)+1] = '\0';
    memcpy(&gs_log_mng_t.logFileInfo.logHead, tmpBuff, sizeof(LOG_HEAD_INFO));
    printf("logHead.logItemNum:%d\n",gs_log_mng_t.logFileInfo.logHead.logItemNum);
    pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);

    pthread_mutex_lock(&mutex);
    thread_initialized = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    //close(fd);
    closedir(dir);

    return 0;
}


int writeLogApi(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen)
{

    LOG_ITEM log_item = {0};
#if defined(USE_POSIX_MQ)
    mqd_t mq;
    struct mq_attr attr = {0};
#else
    MSG_T msg = {0};
#endif
    waitModuleInited();

    /* 组装消息 */

    log_item.major = log_write_param->major;
    log_item.minor = log_write_param->minor;
    log_item.time = log_write_param->time;
    log_item.info_len = dataLen;
    memcpy(log_item.info, data, dataLen);

    int msgid = msgget(KEY, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("msgget");
        return -1;
    }

    msg.mtype = 1;
    msg.data  = log_item;
    if (msgsnd(msgid, &msg, sizeof(LOG_ITEM), IPC_NOWAIT) == -1)
    {
        if (errno == EAGAIN)
        {
            printf("Message queue is full.\n");
        }
        else
        {
            perror("msgsnd");
        }
        return -1;
    }

#if defined(USE_POSIX_MQ)

    mq = mq_open(LOG_QUEUE,/* O_WRONLY */ O_CREAT | O_RDWR | O_NONBLOCK, 0666, NULL);
    if ((mqd_t)-1 == mq) 
    {
        perror("mq_open");
        return -1;
    }
    mq_getattr(mq, &attr);
    //printf("%d-%d-%d-%d\n",attr.mq_maxmsg,attr.mq_msgsize,attr.mq_curmsgs);

    // 发送消息到队列
    if (mq_send(mq, &log_item, sizeof(LOG_ITEM), NULL) == -1) 
    {
        if (errno == EAGAIN) 
        {
            // 消息队列已满，处理相应逻辑
            printf("Message queue is full.\n");
            return -1;
        } 
        else 
        {
            perror("mq_send");
            printf("------------------------------------\n");
            return -1;
        }
    }
    mq_close(mq);
    //printf("mq_send log success!\n");
#endif
    return 0;
}


int writeLog(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen)
{
    char filename[64] = {0};
    static LOG_HEAD_INFO headInfo = {0};
    LOG_ITEM_NODE node = {0};

    waitModuleInited();

    //printf("head len:%d file len:%d log item len:%d\n",sizeof(LOG_HEAD_INFO), sizeof(LOG_FILE_INFO), sizeof(LOG_ITEM));
    /*判断日志是否非法*/
    if(NULL == log_write_param || NULL == data)
    {
        return -1;
    }
    pthread_mutex_lock(&gs_log_mng_t.logMngMutex);

    /* 当前日志文件头信息拷贝到内存中 */
    memcpy(&headInfo, &gs_log_mng_t.logFileInfo.logHead, sizeof(LOG_HEAD_INFO));

    /* 当前没有日志文件 或者当前日志文件已经写满 */
    if(0 == gs_log_mng_t.logFileNum || 10 == gs_log_mng_t.logFileInfo.logHead.logItemNum)
    {
        /* 上一个日志文件已满，关闭文件描述符 */
        if(gs_log_mng_t.logFileInfo.logFilefd > 0)
        {
            close(gs_log_mng_t.logFileInfo.logFilefd);
        }
        /* 当前日志文件日志条目清0 */
        gs_log_mng_t.logFileInfo.logItemNum = 0;

        strncpy(filename,LOG_FOLDER,strlen(LOG_FOLDER)+1);

        /* 构建新的日志文件名 */
        snprintf(filename, sizeof(filename),
                    "%s/log%03d",
                    LOG_FOLDER,
                    gs_log_mng_t.logFileInfo.seq);

        printf("new file:%s\r\n", filename);

        /*创建新的日志文件*/
        gs_log_mng_t.logFileInfo.logFilefd = open(filename, O_CREAT | O_RDWR | O_TRUNC /*| O_APPEND*/, 0600);
        if(gs_log_mng_t.logFileInfo.logFilefd < 0)
        {
            pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
            return -1;
        }

        memset(&gs_log_mng_t.logFileInfo.logHead, 0, sizeof(LOG_HEAD_INFO));

        /* 清空文件头 */
        memset(&headInfo, 0, sizeof(LOG_HEAD_INFO));

        /* 更新文件头信息 */
        headInfo.startTime = log_write_param->time;
        headInfo.endSeq = 0;
        headInfo.fileWriteSeq = 0;

        node.log_item.seq = 1;

        /* 新建文件头信息 */
        lseek(gs_log_mng_t.logFileInfo.logFilefd, 0, SEEK_SET);
        write(gs_log_mng_t.logFileInfo.logFilefd, &headInfo, sizeof(LOG_HEAD_INFO));
        fsync(gs_log_mng_t.logFileInfo.logFilefd);

        headInfo.fileWriteSeq += sizeof(LOG_HEAD_INFO);
        
        /* 更新当前文件信息 */
        gs_log_mng_t.logFileNum++;
        gs_log_mng_t.logFileInfo.seq++;
        //gs_log_mng_t.logFileInfo.logHead.log_set[headInfo.logItemNum].startTime = log_write_param->time;
        //gs_log_mng_t.logFileInfo.logHead.fileWriteSeq += sizeof(LOG_HEAD_INFO) + LOG_ITEM_LEN;
        //memcpy(&gs_log_mng_t.logFileInfo.logHead, &headInfo, sizeof(LOG_HEAD_INFO));

    }
    else 
    {
        if(gs_log_mng_t.logFileInfo.logFilefd < 0) 
        {
            printf("no file valid!\n");
            pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
            return -1;
        }

        //printf("current file:%s\n", gs_log_mng_t.logFileInfo.fileName);  

    }

    /* 写入新的日志条目到日志文件 */
    //printf("offset:%d\n",gs_log_mng_t.logFileInfo.logHead.fileWriteSeq);

    node.log_item.time = log_write_param->time;
    node.log_item.major = log_write_param->major;
    node.log_item.minor = log_write_param->minor;
    memcpy(node.log_item.info, data, LOG_ITEM_LEN-20);

    /* 向日志文件写入日志条目信息 */
    lseek(gs_log_mng_t.logFileInfo.logFilefd, headInfo.fileWriteSeq, SEEK_SET);
    write(gs_log_mng_t.logFileInfo.logFilefd, &node.log_item, sizeof(LOG_ITEM));
    fsync(gs_log_mng_t.logFileInfo.logFilefd);

    gs_log_mng_t.logFileInfo.logItemNum++;
    gs_log_mng_t.logFileInfo.logHead.logItemNum++;


    /* 更新文件头信息 */
    headInfo.endSeq++;
    headInfo.endTime = log_write_param->time; 
    headInfo.logItemNum++;
    headInfo.fileWriteSeq += LOG_ITEM_LEN;

    /* 重写日志文件头 */
    lseek(gs_log_mng_t.logFileInfo.logFilefd, 0, SEEK_SET);
    write(gs_log_mng_t.logFileInfo.logFilefd, &headInfo, sizeof(LOG_HEAD_INFO));
    //fsync(gs_log_mng_t.logFileInfo.logFilefd);
    //headInfo.log_set[headInfo.logItemNum++].startTime = log_write_param->time;

    /* 更新内存中的文件头信息 */
    memcpy(&gs_log_mng_t.logFileInfo.logHead, &headInfo, sizeof(LOG_HEAD_INFO));

    pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);

    //printf("fd:%d\n",gs_log_mng_t.logFileInfo.logFilefd);
    return 0;
}


int logMngTask(void *arg)
{
#if defined(USE_POSIX_MQ) 
    mqd_t mq;
    struct mq_attr attr;
    char tmpBuff[128] = {0};
#else
    MSG_T msg = {0};
#endif
    LOG_WRITE_PARAM log_write_param = {0};
    LOG_ITEM log_item = {0};
    int ret = -1;
    ret = initLogMng();
    if( -1 == ret)
    {
        printf("[%s:%d] initLogMng failed!\n", __FUNCTION__,__LINE__);
    }
    
    waitModuleInited();
#if defined(USE_POSIX_MQ)
    mq = mq_open(LOG_QUEUE, O_RDONLY);
    if (mq == (mqd_t)-1) 
    {
        perror("mq_open");
        exit(1);
    }
    mq_getattr(mq, &attr);
#else
    int msgid = msgget(KEY, IPC_CREAT | 0666);
    if (msgid == -1)
    {
        perror("msgget");
        return -1;
    }
#endif
    //printf("%d-%d-%d\n",attr.mq_maxmsg,attr.mq_msgsize,attr.mq_curmsgs);
    while(1)
    {
#if defined(USE_POSIX_MQ)        
        /* 读取消息必须使用attr.mq_msgsize，否则读不出来 */
        if (mq_receive(mq, tmpBuff, attr.mq_msgsize, NULL) == -1) 
#else
        if(-1 == msgrcv(msgid, &msg, sizeof(LOG_ITEM), 0, IPC_NOWAIT))
#endif
        {
            //printf("mq_receive fail!\n");
            sleep(2);
            continue;
        }

#if !defined(USE_POSIX_MQ)
        log_item = msg.data;
#endif

        /* 组装参数 */
        log_write_param.major = log_item.major;
        log_write_param.minor = log_item.minor;
        log_write_param.time  = log_item.time;

        writeLog(&log_write_param, log_item.info, log_item.info_len);

        sleep(1);
    }
    return 0;
}


//pthread_t startLogMngTask(void)
void startLogMngTask(void)
{
    pthread_t newthread = 0;
    if (pthread_create(&newthread , NULL, (void *)logMngTask, NULL) != 0)
    {
        perror("pthread_create");
    }
    /* 等待线程初始化完成，需要等待socket服务启动，gs_cmd_list初始化完成，
     * 否则外部调用的regShellCmd可能失败
     */
    waitModuleInited();
    //return newthread;
#if 1
    int result;
    /* 设置线程为分离状态, 由系统自动回收线程资源 */
    result = pthread_detach(newthread);
    if (result != 0) {
        perror("Setting thread as detached failed");
        return;
    }
    return;
#endif

}


