#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define APP_NAME "gateWay"
#define APP_PATH "/home/root/gateWay"

int is_process_running(const char *name)
{
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "pidof %s > /dev/null", name);
    return system(cmd) == 0;
}

// void daemonize(void)
// {
//     pid_t pid = fork();
//     if (pid < 0) exit(1);
//     if (pid > 0) exit(0);  // 父进程退出

//     setsid();              // 创建新会话

//     pid = fork();
//     if (pid < 0) exit(1);
//     if (pid > 0) exit(0);  // 防止重新获得终端

//     chdir("/");
//     umask(0);

//     // 关闭标准输入输出
//     fclose(stdin);
//     fclose(stdout);
//     fclose(stderr);
// }

int main(void)
{
    //daemonize();

    while(1)
    {
    printf("222222222222222222222\r\n");
    sleep(1);
    }
    //signal(SIGCHLD, SIG_IGN); // 防止僵尸进程

    // while (1)
    // {
    //     if (!is_process_running(APP_NAME))
    //     {
    //         pid_t pid = fork();

    //         if (pid == 0)
    //         {
    //             execl(APP_PATH, APP_NAME, NULL);
    //             perror("execl failed");
    //             exit(1);
    //         }
    //     }

    //     sleep(3);
    // }

    return 0;
}


