#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include "shellServer.h"

/* 以下变量不要放shellServer.h, 因为放到头文件，编译demo时会警告定义却没有使用 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //互斥锁
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // 条件变量，用于通知主线程子线程已经初始化完成
static int thread_initialized = 0;  // 外部函数需要等待gs_cmd_list初始化才能使用regShellCmd

static struct shellcmdList gs_cmd_list = {0};


static void analysis_cmd(char *recvbuf, int recvlen, char *sendbuf, int sendlen)
{
    struct cmdheader cmd = {0};
    if (NULL == recvbuf || recvlen < sizeof(cmd))
    {
        printf("[%s:%d]invalid param!\n",__FUNCTION__,__LINE__);
        return;
    }

    memcpy(&cmd, recvbuf, sizeof(struct cmdheader));
    SHELLSERVER_PRINT("name:%s argc:%d argvlen:%d length: %d count:%d\n", cmd.cmdname, cmd.argc, cmd.argvlen, cmd.length, lstLength(&gs_cmd_list.list));
    recvbuf += sizeof(cmd);
    pthread_mutex_lock(&gs_cmd_list.mutex);
    shellunit *u = (shellunit *)lstFirst(&gs_cmd_list.list);
    shellunit *next;

    while(u)
    {
        SHELLSERVER_PRINT("u->cmdname:%s\n", u->cmdname);

        next = (shellunit *)lstNext(&u->node);
        if(!strncmp(u->cmdname, cmd.cmdname, strlen(cmd.cmdname)))
        {
            u->fn(u->udata, recvbuf, sendbuf);
        }
        u = next;
    }
    pthread_mutex_unlock(&gs_cmd_list.mutex);
}


int regShellCmd(char *name, Callback fn, void *udata)
{
    if(NULL == name)
    {
        printf("[%s:%d] invalid name!\n",__FUNCTION__,__LINE__);
        return -1;
    }
    shellunit *u;

    u = (shellunit *)malloc(sizeof(shellunit));
    pthread_mutex_lock(&gs_cmd_list.mutex);
    u->fn = fn;
    u->udata = udata;
    strncpy(u->cmdname, name, sizeof(u->cmdname));
    lstAddTail(&gs_cmd_list.list, &u->node);
    pthread_mutex_unlock(&gs_cmd_list.mutex);
    return 0;
}


static int startup()
{
    int serversocket = 0;
    int on = 1;
    struct sockaddr_un name;

    serversocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serversocket == -1)
        perror("socket");
    memset(&name, 0, sizeof(name));
    name.sun_family = AF_UNIX;
    /*删除之前的套接字文件*/
    unlink(SOCKET_PATH); 
    /* 创建用于通信的套接字文件*/
    strncpy(name.sun_path, SOCKET_PATH, sizeof(name.sun_path)-1);

    if ((setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  
    {  
        perror("setsockopt failed");
    }
    if (bind(serversocket, (struct sockaddr *)&name, sizeof(name)) < 0)
        perror("bind");

    if (listen(serversocket, 5) < 0)
        perror("listen");
    return(serversocket);
}


void test_v1(void *udata, char *recvbuf, char *sendbuf)
{
    unsigned int dwLen = 0;
    //printf("%s\n", (char *)udata);
    //printf("%s\n", recvbuf);
    dwLen += snprintf(sendbuf, SHELLBUFFLEN, "this is ");
    dwLen += snprintf(sendbuf+dwLen, SHELLBUFFLEN, "test1!\n");
}


void *shellServerTask(void *arg)
{
    int server_sock = -1;
    int client_sock = -1;
    struct sockaddr_un client_name;
    socklen_t  client_name_len = sizeof(client_name);

    /* 初始化链表头 */
    lstInit(&gs_cmd_list.list);
    
    /* 初始化互斥锁 */
    if (pthread_mutex_init(&gs_cmd_list.mutex, NULL) != 0) 
    {
        perror("Mutex initialization failed");
        //return 1;
    }

    /* 获取用于本机通信的套接字 */
    server_sock = startup();

    /*一下为用于测试的函数 */
    if (-1 == regShellCmd("test1", test_v1, "hello"))
    {
        printf("[%s:%d] invalid name!\n",__FUNCTION__,__LINE__);
    }
     
    /* 线程初始化工作完成后通知等待的主线程 
     * tips: 为什么有cond了还需要thread_initialized
     * 这是为了防止一种叫虚假唤醒的问题, 实测删除了也可以work
     */
    pthread_mutex_lock(&mutex);
    thread_initialized = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    printf("\r\n\033[33m[SHELL] shellserver init!\033[0m\r\n");
    while (1)
    {
        client_sock = accept(server_sock,
                (struct sockaddr *)&client_name,
                &client_name_len);
        if (client_sock == -1)
        {
            perror("accept");         
            continue;
        }
 
        if(read(client_sock, gs_cmd_list.recvbuf, sizeof(gs_cmd_list.recvbuf)))
        {
            //printf("recvbuf:%s\n",gs_cmd_list.recvbuf);
            analysis_cmd(gs_cmd_list.recvbuf, sizeof(gs_cmd_list.recvbuf), gs_cmd_list.sendbuf, sizeof(gs_cmd_list.sendbuf));
            write(client_sock, gs_cmd_list.sendbuf,strlen(gs_cmd_list.sendbuf));
        }
        close(client_sock);
    }
    close(server_sock);
    unlink(SOCKET_PATH);
    pthread_exit(NULL);
    return 0;
}

//pthread_t startShellServerTask(void)
void startShellServerTask(void)
{
    pthread_t newthread = 0;
    pthread_attr_t attr;
    
    /* 初始化线程属性 */
    pthread_attr_init(&attr);

    /* 设置线程属性为守护线程守护线程
     *（Daemon Thread）的资源通常由操作系统来回收。
     * 守护线程是一种在主线程或主进程退出时自动终止的线程。
     * 
     */

    /* 设置线程堆栈大小（以字节为单位）*/
    size_t stack_size = 128 * 1024; // 128KB
    pthread_attr_setstacksize(&attr, stack_size);

    /* 设置线程调度参数 */
    struct sched_param sched_param;
    sched_param.sched_priority = 10; // 优先级范围一般为 1-99，数值越高优先级越高
    pthread_attr_setschedparam(&attr, &sched_param);

    /* 设置线程调度策略 */
    //int policy = SCHED_FIFO; // 先进先出调度策略
    int policy = SCHED_RR; // 时间片轮转
    pthread_attr_setschedpolicy(&attr, policy);

    /* 设置线程分离状态为守护线程 
     * 和pthread_detach相比，以下函数实现了
     * 设置了线程的分离属性，该属性可以被多个线程使用，而pthread
     * 则绑定了一个独立的线程
     */
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) 
    {
        perror("pthread_attr_setdetachstate");
        exit(EXIT_FAILURE);
    }

    /* 创建线程 */
    if (pthread_create(&newthread , &attr, (void *)shellServerTask, NULL) != 0)
            perror("pthread_create");
    
    // 销毁线程属性对象
    pthread_attr_destroy(&attr);
    
    /* 等待线程初始化完成，需要等待socket服务启动，gs_cmd_list初始化完成，
     * 否则外部调用的regShellCmd可能失败
     */
    pthread_mutex_lock(&mutex);
    while (!thread_initialized)
    {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);


    //return newthread;
#if 0 
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
