/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"

class WebServer {
public:
    /*
     * port       : 监听端口
     * trigMode   : epoll 触发模式（0=LT/LT 1=LT/ET 2=ET/LT 3=ET/ET）
     * timeoutMS  : 连接空闲超时（毫秒，0 表示不超时）
     * OptLinger  : 是否启用 SO_LINGER 优雅关闭
     * threadNum  : 工作线程数
     * openLog    : 是否开启日志
     * logLevel   : 日志等级（0=DEBUG 1=INFO 2=WARN 3=ERROR）
     * logQueSize : 异步日志队列容量（0 表示同步写）
     */
    WebServer(int port, int trigMode, int timeoutMS, bool OptLinger,
              int threadNum,
              bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void Start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;
    static int SetFdNonblock(int fd);

    int  port_;
    bool openLinger_;
    int  timeoutMS_;
    bool isClose_;
    int  listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer>  timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller>    epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif // WEBSERVER_H
