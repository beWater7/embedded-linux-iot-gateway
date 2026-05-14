/*
 * @Author       : mark
 * @Date         : 2020-06-18
 * @copyleft Apache 2.0
 */
#include <unistd.h>
#include "server/webserver.h"

int main()
{
    /* 守护进程后台运行（需要时取消注释） */
    // daemon(1, 0);

    /*
     * 参数说明：
     *   port      = 1316   监听端口
     *   trigMode  = 3      listenFd ET + connFd ET（嵌入式高并发推荐）
     *   timeoutMS = 60000  连接空闲超时 60s
     *   OptLinger = false  不启用 SO_LINGER
     *   threadNum = 4      工作线程（imx6ull 双核，4 线程够用）
     *   openLog   = true
     *   logLevel  = 1      INFO 级别
     *   logQueSize= 512    异步日志队列
     */
    WebServer server(
        1316, 3, 60000, false,
        /*threadNum=*/4,
        /*openLog=*/true, /*logLevel=*/1, /*logQueSize=*/512);

    server.Start();
    return 0;
}
