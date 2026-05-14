/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */
#include "webserver.h"
#include "../auth/userstore.h"

using namespace std;

WebServer::WebServer(int port, int trigMode, int timeoutMS, bool OptLinger,
                     int threadNum,
                     bool openLog, int logLevel, int logQueSize)
    : port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
      timer_(new HeapTimer()),
      threadpool_(new ThreadPool(threadNum)),
      epoller_(new Epoller())
{
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);

    HttpConn::userCount = 0;
    HttpConn::srcDir    = srcDir_;

    /* 加载用户表（路径相对于运行目录） */
    UserStore::LoadFromFile("resources/users.conf");

    InitEventMode_(trigMode);
    if (!InitSocket_()) { isClose_ = true; }

    if (openLog) {
        Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
        if (isClose_) {
            LOG_ERROR("========== Server init error!==========");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET" : "LT"),
                     (connEvent_   & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("ThreadPool num: %d", threadNum);
        }
    }
}

WebServer::~WebServer()
{
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::InitEventMode_(int trigMode)
{
    listenEvent_ = EPOLLRDHUP;
    connEvent_   = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode) {
    case 0: break;
    case 1: connEvent_   |= EPOLLET; break;
    case 2: listenEvent_ |= EPOLLET; break;
    case 3:
    default:
        listenEvent_ |= EPOLLET;
        connEvent_   |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::Start()
{
    int timeMS = -1;
    if (!isClose_) { LOG_INFO("========== Server start =========="); }
    while (!isClose_) {
        if (timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++) {
            int      fd     = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listenFd_) {
                DealListen_();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            } else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            } else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char* info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_,
                    std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if (fd <= 0) { return; }
        if (HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn* client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn* client)
{
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn* client)
{
    assert(client);
    if (timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

void WebServer::OnRead_(HttpConn* client)
{
    assert(client);
    int ret       = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn* client)
{
    if (client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client)
{
    assert(client);
    int ret        = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

bool WebServer::InitSocket_()
{
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port_);

    struct linger optLinger = {0, 0};
    if (openLinger_) {
        optLinger.l_onoff  = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Create socket error!");
        return false;
    }

    if (setsockopt(listenFd_, SOL_SOCKET, SO_LINGER,
                   &optLinger, sizeof(optLinger)) < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!");
        return false;
    }

    int optval = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR,
                   (const void*)&optval, sizeof(int)) == -1) {
        close(listenFd_);
        LOG_ERROR("set socket setsockopt error!");
        return false;
    }

    if (bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listenFd_);
        LOG_ERROR("Bind Port:%d error!", port_);
        return false;
    }

    if (listen(listenFd_, 6) < 0) {
        close(listenFd_);
        LOG_ERROR("Listen port:%d error!", port_);
        return false;
    }

    if (epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN) == 0) {
        close(listenFd_);
        LOG_ERROR("Add listen error!");
        return false;
    }

    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int WebServer::SetFdNonblock(int fd)
{
    assert(fd > 0);
    /* 修正原实现的 F_GETFD → F_GETFL，才能正确设置 O_NONBLOCK */
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
