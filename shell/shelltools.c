#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>

//#define DEBUG
#define SOCKET_PATH "/tmp/mysocket"

struct cmdheader{
    char cmdname[16];
    int argc;    // 参数个数
    int argvlen; // 参数长度
    int length;  // 所有长度
};


int main(int argc, char *argv[])
{
    int i,j;
    int sockfd;
    int len;
    struct sockaddr_un address;
    int result;
    char recvbuf[64] = {0};
    char sendbuf[64] = {0};    
    int output[2]; //管道 
    struct cmdheader cmd = {0};
    int flag = 0;
    int bytesRead = 0;

    //strncpy(cmd.cmdname, argv[0], strlen(argv[0]));
    if(argc >= 2 && !strncmp(argv[0], "./shelltools", strlen("./shelltools")))
    {
        flag = 1;
        strncpy(cmd.cmdname, argv[1], sizeof(cmd.cmdname));
    }
    else
    {
        if(strstr(argv[0], "./"))
        {
            argv[0] += strlen("./");
        }
        strncpy(cmd.cmdname, argv[0], sizeof(cmd.cmdname));
    }
    
    cmd.cmdname[sizeof(cmd.cmdname)-1] = '\0'; //

    cmd.argc = argc; // 加上了cmd
    for(i = 0; i < argc; i++)
    {
        cmd.argvlen += strlen(argv[i]);
    }
    cmd.length += cmd.argvlen;
#ifdef DEBUG
    printf("name:%s argc:%d argvlen:%d length: %d\n", cmd.cmdname, cmd.argc, cmd.argvlen, cmd.length);
#endif
    memcpy(sendbuf, &cmd, sizeof(cmd)); // 结构体用memcpy
#ifdef DEBUG
    for(j = 0; j < sizeof(sendbuf); j++)
    {
        printf("%02x ",sendbuf[j]);
    }
    printf("\n");
#endif
    strncpy(sendbuf+sizeof(cmd), argv[0], strlen(argv[0])); 

    if( (flag && 3 == i) || (0 == flag && 2 == i))
    {   
        strncpy(sendbuf+sizeof(cmd) + strlen(argv[0]), " ", strlen(" ")); // 中间以空格为分割符
        strncpy(sendbuf+sizeof(cmd)+strlen(argv[0]) + 1, argv[i-1], strlen(argv[i-1])); // 只支持用户输入一个参数
    }
#ifdef DEBUG
    for(j = 0; j < sizeof(sendbuf); j++)
    {
        printf("%02x ",sendbuf[j]);
    }
    printf("\n");
#endif
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0); // AF_UNIX用于本机进程间通信
    if(-1 == sockfd)
    {
        perror("socket");
        exit(1);
    }
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path)-1);
    len = sizeof(address);
    result = connect(sockfd, (struct sockaddr *)&address, len);
    if (result == -1)
    {
        perror("oops: client1");
        exit(1);
    }
    /* 向shellServer发送数据 */
    write(sockfd, sendbuf, sizeof(sendbuf));

    /* 读取shellServer发送的数据 */
    do{
        memset(recvbuf, 0, sizeof(recvbuf));
        //bytesRead = read(sockfd, recvbuf, sizeof(recvbuf));
        bytesRead = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
        // write(sockfd, output[1], sizeof(recvbuf));
        // read(output[0], recvbuf, sizeof(recvbuf));
        if(bytesRead > 0)
        {
            printf("%s", recvbuf);
        }
#if 1
        else if(0 == bytesRead)
        {
            //printf("[%s:%d] no data recvied!\n",__FUNCTION__,__LINE__);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            
            //printf("[%s:%d] no data!\n",__FUNCTION__,__LINE__);
            // 当前没有数据可读
            // 可以继续做其他事情，或者等待下次读取
        }
#endif 
        else 
        {
            perror("recv");
            break;
        }
    }while(bytesRead > 0);

    //printf("char from server = %s\n", recvbuf);

    close(sockfd);
    exit(0);
}
