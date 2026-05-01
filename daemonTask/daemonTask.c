#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <spawn.h>
#include <time.h>

#define APP_DIR "/home/root/"
#define APP_NAME "gateWay"

#define APP_PATH APP_DIR APP_NAME
extern char **environ;

int is_process_running(const char *name)
{
    FILE *fp = fopen("/var/run/gateway.pid", "r");
    if (!fp) return 0;

    int pid;
    fscanf(fp, "%d", &pid);
    fclose(fp);

    // 检查进程是否存在
    if (kill(pid, 0) == 0)
        return 1;

    return 0;
}


void daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);  // 父进程退出

    setsid();              // 创建新会话

    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);  // 防止重新获得终端

    chdir("/");
    umask(0);

    // 关闭标准输入输出
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
}

int main(void)
{
    //daemonize();
    signal(SIGCHLD, SIG_IGN); // 防止僵尸进程

    int restart_count = 0;
    time_t last_time = 0;

    while (1)
    {
        if (!is_process_running(APP_NAME))
        {
            #if 0
            /* app 作为守护进程的子进程 */
            pid_t pid = fork();

            if (pid == 0)
            {
                execl(APP_PATH, APP_NAME, NULL);
                perror("execl failed");
                exit(1);
            }
            #endif
            
            time_t now = time(NULL);

            // ⭐ 限频机制（关键）
            if (now - last_time < 5)
            {
                restart_count++;
            }
            else
            {
                restart_count = 0;
            }

            last_time = now;

            if (restart_count > 5)
            {
                sleep(10); // 防炸
                restart_count = 0;
            }

            /* 启动进程（推荐 posix_spawn）系统优化的fork+exec, libc+linux2.6以上支持 */
            pid_t pid;
            char *argv[] = {"gateWay", NULL};

            int ret = posix_spawn(&pid, "/home/root/gateWay",
                                 NULL, NULL, argv, environ);

            if (ret != 0)
            {
                // 写日志而不是 printf
            }
            
        }

        sleep(3);
    }

    return 0;
}