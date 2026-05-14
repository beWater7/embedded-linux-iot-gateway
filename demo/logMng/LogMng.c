/*
 * LogMng.c — 异步日志管理模块
 *
 * 架构：
 *   写入方：调用 writeLogApi()，将 LOG_ITEM 通过 System V 消息队列发送。
 *   消费方：logMngTask() 线程阻塞读取队列，依次调用 writeLog() 落盘。
 *
 * 使用前必须先调用 startLogMngTask()，之后才能调用 writeLogApi()。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "LogMng.h"

/* System V 消息队列 key，同一主机内唯一标识本模块的队列 */
#define LOGMNG_MSG_KEY  1234

/* ------------------------------------------------------------------ */
/* 模块级全局状态                                                        */
/* ------------------------------------------------------------------ */

static pthread_mutex_t s_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_init_cond  = PTHREAD_COND_INITIALIZER;
static int             s_init_done  = 0;   /* 0:未完成  1:已完成（成功或失败） */

LOG_MNG_T gs_log_mng_t;

/* ------------------------------------------------------------------ */
/* 内部辅助函数                                                          */
/* ------------------------------------------------------------------ */

/* 阻塞等待初始化完成（成功与失败均会被唤醒） */
static void waitModuleInited(void)
{
    pthread_mutex_lock(&s_init_mutex);
    while (!s_init_done)
        pthread_cond_wait(&s_init_cond, &s_init_mutex);
    pthread_mutex_unlock(&s_init_mutex);
}

/* 通知所有等待方：初始化已完成 */
static void notifyModuleInited(void)
{
    pthread_mutex_lock(&s_init_mutex);
    s_init_done = 1;
    pthread_cond_broadcast(&s_init_cond);
    pthread_mutex_unlock(&s_init_mutex);
}

/* qsort 比较函数 */
static int compareFileName(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

/* 判断文件名是否符合 logNNN 规范 */
static int isLogFileName(const char *name)
{
    unsigned int dummy;
    return (sscanf(name, "log%u", &dummy) == 1);
}

/* ------------------------------------------------------------------ */
/* initLogMng                                                           */
/*   在消费者线程中调用：创建消息队列、扫描磁盘文件、恢复内存状态。          */
/*   无论成功与否，函数末尾统一调用 notifyModuleInited()。                 */
/* ------------------------------------------------------------------ */
static int initLogMng(void)
{
    DIR           *dir        = NULL;
    struct dirent *entry      = NULL;
    struct stat    st;
    char         **files      = NULL;
    int            file_count = 0;
    LOG_NODE      *log_node   = NULL;
    int            ret        = 0;

    memset(&gs_log_mng_t, 0, sizeof(LOG_MNG_T));
    gs_log_mng_t.logFileInfo.logFilefd = -1;
    lstInit(&gs_log_mng_t.logFileList);

    if (pthread_mutex_init(&gs_log_mng_t.logMngMutex, NULL) != 0)
    {
        perror("[LogMng] mutex_init");
        ret = -1;
        goto done;
    }

    /* 创建（或复用）System V 消息队列 */
    gs_log_mng_t.MsgQid = msgget(LOGMNG_MSG_KEY, IPC_CREAT | 0666);
    if (gs_log_mng_t.MsgQid == -1)
    {
        perror("[LogMng] msgget");
        ret = -1;
        goto done;
    }

    /* 打开或创建日志目录 */
    dir = opendir(LOG_FOLDER);
    if (dir == NULL)
    {
        if (errno == ENOENT)
        {
            if (mkdir(LOG_FOLDER, 0755) == -1 && errno != EEXIST)
            {
                perror("[LogMng] mkdir");
                ret = -1;
                goto done;
            }
            dir = opendir(LOG_FOLDER);
        }
        if (dir == NULL)
        {
            perror("[LogMng] opendir");
            ret = -1;
            goto done;
        }
    }

    /* 遍历目录，只收集符合 logNNN 命名规范的普通文件 */
    while ((entry = readdir(dir)) != NULL)
    {
        if (!isLogFileName(entry->d_name))
            continue;

        char fullpath[PATH_MAX];
        if (snprintf(fullpath, sizeof(fullpath), "%s/%s",
                     LOG_FOLDER, entry->d_name) >= (int)sizeof(fullpath))
            continue;

        if (stat(fullpath, &st) == -1 || !S_ISREG(st.st_mode))
            continue;

        char **tmp = realloc(files, (file_count + 1) * sizeof(char *));
        if (!tmp)
        {
            perror("[LogMng] realloc");
            ret = -1;
            goto cleanup;
        }
        files = tmp;

        files[file_count] = strdup(entry->d_name);
        if (!files[file_count])
        {
            perror("[LogMng] strdup");
            ret = -1;
            goto cleanup;
        }
        file_count++;
    }

    /* 按文件名字典序排序（log000 < log001 < ...） */
    qsort(files, file_count, sizeof(char *), compareFileName);

    /* 构建管理链表，同时提取已知最大 seq 值 */
    int max_seq = -1;
    for (int i = 0; i < file_count; i++)
    {
        log_node = (LOG_NODE *)malloc(sizeof(LOG_NODE));
        if (!log_node)
        {
            perror("[LogMng] malloc LOG_NODE");
            ret = -1;
            goto cleanup;
        }
        memset(log_node, 0, sizeof(LOG_NODE));
        strncpy(log_node->fileName, files[i], NAME_MAX_LEN - 1);
        lstAddTail(&gs_log_mng_t.logFileList, &log_node->node);
        gs_log_mng_t.logFileNum++;

        int s;
        if (sscanf(files[i], "log%d", &s) == 1 && s > max_seq)
            max_seq = s;
    }

    /*
     * 下一个文件序号 = 磁盘上已知最大序号 + 1。
     * 这样即使中间某个文件被删除，也不会产生重名文件。
     */
    gs_log_mng_t.logFileInfo.seq = (max_seq >= 0) ? (max_seq + 1) : 0;

    /* 打开最后一个文件并读取其文件头，以恢复写入位置 */
    if (gs_log_mng_t.logFileNum > 0)
    {
        log_node = (LOG_NODE *)lstLast(&gs_log_mng_t.logFileList);

        char dstFileName[PATH_MAX];
        if (snprintf(dstFileName, sizeof(dstFileName), "%s/%s",
                     LOG_FOLDER, log_node->fileName) >= (int)sizeof(dstFileName))
        {
            fprintf(stderr, "[LogMng] path too long\n");
            ret = -1;
            goto cleanup;
        }

        int fd = open(dstFileName, O_RDWR);
        if (fd < 0)
        {
            perror("[LogMng] open last log");
            ret = -1;
            goto cleanup;
        }

        pthread_mutex_lock(&gs_log_mng_t.logMngMutex);
        gs_log_mng_t.logFileInfo.logFilefd = fd;
        ssize_t n = read(fd, &gs_log_mng_t.logFileInfo.logHead, sizeof(LOG_HEAD_INFO));
        if (n != (ssize_t)sizeof(LOG_HEAD_INFO))
        {
            fprintf(stderr, "[LogMng] header read failed (n=%zd, expected %zu)\n",
                    n, sizeof(LOG_HEAD_INFO));
            close(fd);
            gs_log_mng_t.logFileInfo.logFilefd = -1;
            pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
            ret = -1;
            goto cleanup;
        }
        pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
    }

cleanup:
    if (files)
    {
        for (int i = 0; i < file_count; i++)
            free(files[i]);
        free(files);
    }
    if (dir)
        closedir(dir);

done:
    gs_log_mng_t.init_result = ret;
    notifyModuleInited();
    return ret;
}

/* ------------------------------------------------------------------ */
/* writeLog (static)                                                    */
/*   由消费者线程独占调用，将日志条目落盘。                                */
/*   含自动轮转逻辑：文件满时创建新文件；超过 LOG_FILE_MAX_NUM 时删除最旧。 */
/* ------------------------------------------------------------------ */
static int writeLog(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen)
{
    char          filename[PATH_MAX] = {0};
    LOG_HEAD_INFO headInfo           = {0};
    LOG_ITEM_NODE node               = {0};
    int           ret                = 0;

    if (!log_write_param || !data || dataLen <= 0)
        return -1;

    pthread_mutex_lock(&gs_log_mng_t.logMngMutex);

    memcpy(&headInfo, &gs_log_mng_t.logFileInfo.logHead, sizeof(LOG_HEAD_INFO));

    /* 第一条日志，或当前文件已写满 → 需要创建新文件 */
    if (gs_log_mng_t.logFileNum == 0 ||
        headInfo.logItemNum >= MAX_LOG_ITEMS_PER_FILE)
    {
        /* 关闭旧文件 */
        if (gs_log_mng_t.logFileInfo.logFilefd >= 0)
        {
            close(gs_log_mng_t.logFileInfo.logFilefd);
            gs_log_mng_t.logFileInfo.logFilefd = -1;
        }

        /* 超过文件数量上限：删除最旧的一个（日志轮转） */
        if (gs_log_mng_t.logFileNum >= LOG_FILE_MAX_NUM)
        {
            NODE *oldest_raw = lstFirst(&gs_log_mng_t.logFileList);
            if (oldest_raw)
            {
                LOG_NODE *oldest = (LOG_NODE *)oldest_raw;
                char old_path[PATH_MAX];
                snprintf(old_path, sizeof(old_path), "%s/%s",
                         LOG_FOLDER, oldest->fileName);
                lstDel(&gs_log_mng_t.logFileList, oldest_raw); /* 内部 free */
                gs_log_mng_t.logFileNum--;
                unlink(old_path);
            }
        }

        /* 构建新文件名 */
        if (snprintf(filename, sizeof(filename), "%s/log%03d",
                     LOG_FOLDER, gs_log_mng_t.logFileInfo.seq) >= (int)sizeof(filename))
        {
            fprintf(stderr, "[LogMng] filename too long\n");
            ret = -1;
            goto unlock;
        }

        gs_log_mng_t.logFileInfo.logFilefd = open(filename, O_CREAT | O_RDWR, 0600);
        if (gs_log_mng_t.logFileInfo.logFilefd < 0)
        {
            perror("[LogMng] open new log");
            ret = -1;
            goto unlock;
        }

        /* 初始化并写入文件头 */
        memset(&headInfo, 0, sizeof(LOG_HEAD_INFO));
        headInfo.startTime    = log_write_param->time;
        headInfo.fileWriteSeq = sizeof(LOG_HEAD_INFO);

        if (write(gs_log_mng_t.logFileInfo.logFilefd,
                  &headInfo, sizeof(LOG_HEAD_INFO)) != (ssize_t)sizeof(LOG_HEAD_INFO))
        {
            perror("[LogMng] write new header");
            close(gs_log_mng_t.logFileInfo.logFilefd);
            gs_log_mng_t.logFileInfo.logFilefd = -1;
            ret = -1;
            goto unlock;
        }
        fsync(gs_log_mng_t.logFileInfo.logFilefd);

        /* 更新全局状态 */
        gs_log_mng_t.logFileNum++;
        gs_log_mng_t.logFileInfo.seq++;
        gs_log_mng_t.logFileInfo.logItemNum = 0;

        /* 将新文件加入管理链表 */
        const char *base = strrchr(filename, '/');
        base = base ? base + 1 : filename;
        LOG_NODE *new_node = (LOG_NODE *)malloc(sizeof(LOG_NODE));
        if (new_node)
        {
            memset(new_node, 0, sizeof(LOG_NODE));
            strncpy(new_node->fileName, base, NAME_MAX_LEN - 1);
            lstAddTail(&gs_log_mng_t.logFileList, &new_node->node);
        }
    }

    /* 组装日志条目 */
    node.log_item.time  = log_write_param->time;
    node.log_item.major = log_write_param->major;
    node.log_item.minor = log_write_param->minor;

    int copy_len = (dataLen > (int)(sizeof(node.log_item.info) - 1))
                   ? (int)(sizeof(node.log_item.info) - 1) : dataLen;
    memcpy(node.log_item.info, data, copy_len);
    node.log_item.info[copy_len] = '\0';
    node.log_item.info_len = (unsigned int)copy_len;

    /* 写入日志条目 */
    if (lseek(gs_log_mng_t.logFileInfo.logFilefd,
              headInfo.fileWriteSeq, SEEK_SET) == -1)
    {
        perror("[LogMng] lseek item");
        ret = -1;
        goto unlock;
    }
    if (write(gs_log_mng_t.logFileInfo.logFilefd,
              &node.log_item, sizeof(LOG_ITEM)) != (ssize_t)sizeof(LOG_ITEM))
    {
        perror("[LogMng] write item");
        ret = -1;
        goto unlock;
    }
    fsync(gs_log_mng_t.logFileInfo.logFilefd);

    /* 更新文件头（endTime / logItemNum / fileWriteSeq） */
    headInfo.endTime = log_write_param->time;
    headInfo.logItemNum++;
    headInfo.fileWriteSeq += sizeof(LOG_ITEM);

    if (lseek(gs_log_mng_t.logFileInfo.logFilefd, 0, SEEK_SET) == -1)
    {
        perror("[LogMng] lseek header");
        ret = -1;
        goto unlock;
    }
    if (write(gs_log_mng_t.logFileInfo.logFilefd,
              &headInfo, sizeof(LOG_HEAD_INFO)) != (ssize_t)sizeof(LOG_HEAD_INFO))
    {
        perror("[LogMng] write header update");
        ret = -1;
        goto unlock;
    }

    /* 同步内存状态 */
    memcpy(&gs_log_mng_t.logFileInfo.logHead, &headInfo, sizeof(LOG_HEAD_INFO));
    gs_log_mng_t.logFileInfo.logItemNum++;

unlock:
    pthread_mutex_unlock(&gs_log_mng_t.logMngMutex);
    return ret;
}

/* ------------------------------------------------------------------ */
/* writeLogApi                                                          */
/*   对外公开接口：将一条日志投递到消息队列（非阻塞，队列满时丢弃并返回-1）。 */
/* ------------------------------------------------------------------ */
int writeLogApi(LOG_WRITE_PARAM *log_write_param, char *data, int dataLen)
{
    MSG_T    msg      = {0};
    LOG_ITEM log_item = {0};

    if (!log_write_param || !data || dataLen <= 0)
        return -1;

    waitModuleInited();

    if (gs_log_mng_t.init_result != 0)
    {
        fprintf(stderr, "[LogMng] module init failed, log dropped.\n");
        return -1;
    }

    log_item.major = log_write_param->major;
    log_item.minor = log_write_param->minor;
    log_item.time  = log_write_param->time;

    /* 安全截断，防止溢出 */
    int copy_len = (dataLen > (int)(sizeof(log_item.info) - 1))
                   ? (int)(sizeof(log_item.info) - 1) : dataLen;
    memcpy(log_item.info, data, copy_len);
    log_item.info[copy_len] = '\0';
    log_item.info_len = (unsigned int)copy_len;

    msg.mtype = 1;
    msg.data  = log_item;

    if (msgsnd(gs_log_mng_t.MsgQid, &msg, sizeof(LOG_ITEM), IPC_NOWAIT) == -1)
    {
        if (errno == EAGAIN)
            fprintf(stderr, "[LogMng] queue full, log dropped.\n");
        else
            perror("[LogMng] msgsnd");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* logMngTask                                                           */
/*   消费者线程：初始化后阻塞读队列并落盘，直到进程结束。                   */
/* ------------------------------------------------------------------ */
static void *logMngTask(void *arg)
{
    (void)arg;

    if (initLogMng() != 0)
    {
        /* notifyModuleInited 已在 initLogMng 内部调用，此处直接退出 */
        fprintf(stderr, "[LogMng] initLogMng failed, task exiting.\n");
        return NULL;
    }

    MSG_T           msg             = {0};
    LOG_WRITE_PARAM log_write_param = {0};

    while (1)
    {
        memset(&msg, 0, sizeof(msg));

        /* 阻塞等待消息（去掉 IPC_NOWAIT），避免轮询空转 */
        if (msgrcv(gs_log_mng_t.MsgQid, &msg, sizeof(LOG_ITEM), 0, 0) == -1)
        {
            if (errno == EINTR)
                continue;          /* 被信号打断，重试 */
            perror("[LogMng] msgrcv");
            sleep(1);              /* 持续错误时避免 CPU 空转 */
            continue;
        }

        log_write_param.major = msg.data.major;
        log_write_param.minor = msg.data.minor;
        log_write_param.time  = msg.data.time;

        writeLog(&log_write_param, msg.data.info, (int)msg.data.info_len);
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* startLogMngTask                                                      */
/*   创建消费者线程并等待其初始化完成。                                    */
/*   必须在 writeLogApi 之前调用。                                        */
/* ------------------------------------------------------------------ */
void startLogMngTask(void)
{
    pthread_t tid = 0;

    if (pthread_create(&tid, NULL, logMngTask, NULL) != 0)
    {
        perror("[LogMng] pthread_create");
        /* 线程未创建，主动置失败标记并通知，防止调用方在 writeLogApi 中永久阻塞 */
        gs_log_mng_t.init_result = -1;
        notifyModuleInited();
        return;
    }

    /* 阻塞直到消费者线程完成初始化（成功或失败） */
    waitModuleInited();

    /* 设为分离状态，由系统自动回收资源 */
    pthread_detach(tid);
}
