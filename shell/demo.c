#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "shellServer.h"
#include "LogMng.h"
#include <time.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


void test3(void *udata, char *recvbuf, char *sendbuf)
{
    unsigned int dwLen = 0;
    dwLen += snprintf(sendbuf, SHELLBUFFLEN, "this is ");
    dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "test3!\n");
}

void setDebugLevel(void *udata, char *recvbuf, char *sendbuf)
{
    
    unsigned int dwLen = 0;
    unsigned int level = 0;
    char *tmpStr = NULL;
    dwLen += snprintf(sendbuf, SHELLBUFFLEN, "recvbuf:%s\n", recvbuf);
    tmpStr = strstr(recvbuf, " ");
    if(NULL == tmpStr)
    {
        dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "debuglevel:%d\n",log_get_level());
        dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "0-LOG_TRACE  1-LOG_DEBUG  2-LOG_INFO\n3-LOG_WARN  4-LOG_ERROR  5-LOG_FATAL\n");
        return;
    }
    tmpStr += strlen(" ");
    sscanf(tmpStr, "%u", &level);
    if(level < 0 || level > 5)
    {
        dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "invalid param!\n");
        dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "debuglevel:%d\n",log_get_level());
        return;
    }
    log_set_level(level); 
    dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "debuglevel: %d\n", level);
}

void getDebugLevel(void *udata, char *recvbuf, char *sendbuf)
{
    u_int32_t dwLen = 0;
    dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "debuglevel:%d\n",log_get_level());
}


struct shellcmd
{
    char cmdname[16];
    //void (*Callback)(void*, char*, char *);
    Callback fn;
    void *udata;
};

/* shell cmd list */
struct shellcmd cmd_list[] = {
    {"setDebugLevel", setDebugLevel, NULL},
    {"getDebugLevel", getDebugLevel, NULL},
};

static int reg_shellcmd_init()
{
    int i = 0;
    for(i = 0; i < ARRAY_SIZE(cmd_list) ; i++)
    {
        if(-1 == regShellCmd(cmd_list[i].cmdname, cmd_list[i].fn, cmd_list[i].udata))
        {
            log_warn("reg shellcmd:%s fail!\n", cmd_list[i].cmdname);
            return -1;
        }
    }
    return 0;
}

int demo_main(void)
{
    pthread_t newthread;
    LOG_WRITE_PARAM log_param;
    /* 启动shellServer线程 */
    startShellServerTask(); 

#if 0
    /* 需要在线程启动后注册指令，否则无法正常注册 */
    if( -1 == regShellCmd("test3", test3, NULL))
    {
        printf("[%s:%d] regShellCmd error!\n",__FUNCTION__,__LINE__);
    }

    if( -1 == regShellCmd("setDebugLevel", setDebugLevel, NULL))
    {
        printf("[%s:%d] regShellCmd error!\n",__FUNCTION__,__LINE__);
    }
#endif
    if(-1 == reg_shellcmd_init())
    {
        log_warn("[%s:%d] reg shellcmd fail!\n",__FUNCTION__,__LINE__);
    }

    #if 0
	// 使用 system 函数调用脚本
    int result = system("./start.sh");
    // 检查调用结果
    if (result == -1) {
        // system 函数调用失败
        perror("system");
        return 0;
    }
    #endif


    /* 新增代码增加在此处 */
    startLogMngTask();

    log_param.major = EVENT;
    log_param.minor = 0x0101;
    time(&log_param.time); 

    if(-1 == writeLogApi(&log_param, "hello world!\n", strlen("hello world!\n")+1))
    {
       log_error("write log failed!\n");
    }
    // writeLogApi(&log_param, "The melody of raindrops on a quiet night is nature's lullaby,  \
    // soothing the restless souls and painting dreams in the hearts of dreamers\n", 100);
    // writeLog(&log_param, "1!\n", 4);
    // writeLog(&log_param, "1!\n", 4);
    // writeLog(&log_param, "1!\n", 4);
    // writeLog(&log_param, "1!\n", 4);

    log_warn("main init end!");
    return 0;
}

int main()
{
    demo_main();
    while(1)
    {
        sleep(10);
    }
    return 0;
}
