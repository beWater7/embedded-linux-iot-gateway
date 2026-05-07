/*************************************************************************
    > File Name: test.c
    > Author: DY
    > Mail: 1781405401@qq.com 
    > Created Time: 2023年12月18日 星期一 23时52分45秒
 ************************************************************************/

#include<stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MSG_SIZE 256
#define QUEUE_NAME "/my_queue"

void *sender_thread(void *arg) {
    mqd_t mq;
    struct mq_attr attr;
	attr.mq_msgsize = 128;
	attr.mq_maxmsg = 10;
    char msg[64] = "Hello from sender!";

    // 打开或创建消息队列
    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open (sender)");
        exit(EXIT_FAILURE);
    }
	printf("send mq:%d\n",mq);
    // 发送消息到队列
    if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
        perror("mq_send (sender)");
        exit(EXIT_FAILURE);
    }
	mq_getattr(mq, &attr);
	printf("%d-%d-%d",attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
    // 关闭消息队列
    mq_close(mq);

    return NULL;
}

void *receiver_thread(void *arg) {
    mqd_t mq;
    char msg[64];
	struct mq_attr attr;

    // 打开消息队列
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0666, NULL);
    if (mq == (mqd_t)-1) {
        perror("mq_open (receiver)");
        exit(EXIT_FAILURE);
    }

	printf("receive mq:%d\n",mq);
	mq_getattr(mq, &attr); // 获取消息队列属性
    // 接收消息
    if (mq_receive(mq, msg, attr.mq_msgsize, NULL) == -1) {
        perror("mq_receive (receiver)");
        exit(EXIT_FAILURE);
    }

    // 输出接收到的消息
    printf("Received message: %s\n", msg);

    // 关闭消息队列
    mq_close(mq);

    return NULL;
}

int main() {
    pthread_t sender, receiver;

    // 创建发送者和接收者线程
    pthread_create(&sender, NULL, sender_thread, NULL);
    pthread_create(&receiver, NULL, receiver_thread, NULL);

    // 等待线程完成
    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    // 删除消息队列
    mq_unlink(QUEUE_NAME);

    return 0;
}

