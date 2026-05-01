/** 环形队列（缓冲区） by liudayi5
 *
 * 2022/10/22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h> // struct iovec 头文件支持
#include "buffer.h"


/* 环形缓冲区初始化 */
int ring_queue_init(RING_QUEUE *ring_que)
{
	ring_que->front = 0;
	ring_que->rear = 0;
	ring_que->readPos = 0;
	ring_que->writePos = 0;
	ring_que->size = RING_QUEUE_SIZE;
    return 0;
}

/* 返回可读的大小 */
size_t readableBytes(const RING_QUEUE *buffer) {
    return buffer->writePos - buffer->readPos;
}

/* 返回可写的大小 */
size_t writableBytes(const RING_QUEUE *buffer) {
    return buffer->size - buffer->writePos;
}

/* 返回缓冲区预留的字节数 */
size_t prependableBytes(const RING_QUEUE*buffer) {
    return buffer->readPos;
}

/* 指向可读数据的指针 */
const ELEMENT *peek(const RING_QUEUE *buffer) {
    return buffer->data + buffer->readPos;
}

/* 根据给定的长度移动读指针 */
void retrieve(RING_QUEUE *buffer, size_t len) {
    assert(len <= readableBytes(buffer));
    buffer->readPos += len;
}

/* 移动读指针到指定位置：end */
void retrieveUntil(RING_QUEUE *buffer, const char *end) {
    assert(peek(buffer) <= end);
    retrieve(buffer, end - peek(buffer));
}

/* 清空缓存区 */
void retrieveAll(RING_QUEUE *buffer) {
    memset(buffer->data, 0, buffer->size);
    buffer->readPos = 0;
    buffer->writePos = 0;
}

/* 将可读数据转换为字符串，并清空缓存区 */
void retrieveAllToStr(RING_QUEUE *buffer, char *output) {
    memcpy(output, peek(buffer), readableBytes(buffer));
    retrieveAll(buffer);
}

/* 返回可写的指针 */
const ELEMENT *beginWriteConst(const RING_QUEUE *buffer) {
    return buffer->data + buffer->writePos;
}

/* 返回可写的指针 */
ELEMENT *beginWrite(RING_QUEUE *buffer) {
    return buffer->data + buffer->writePos;
}

/* 根据写入的长度移动写指针 */
void hasWritten(RING_QUEUE *buffer, size_t len) {
    buffer->writePos += len;
}

ELEMENT * BeginPtr(RING_QUEUE *buffer) {
    return buffer->data;
}

/* 环形缓冲区永远能写入，但是最早的元素将被删除 */
void append(RING_QUEUE *buffer, const char *str, size_t len) {
    assert(str);
    /* 如果当前写入的内容超出缓冲区可写区域大小 */
    if(writableBytes(buffer) < len)
    {
        size_t extraLen = len - writableBytes(buffer);
        printf("no writeable room, delete old data!\n");
        memmove(BeginPtr(buffer), BeginPtr(buffer) + extraLen, buffer->size - extraLen);
        buffer->writePos -= extraLen;
    }
    memcpy(beginWrite(buffer), str, len);
    hasWritten(buffer, len);
}

/* 向缓冲区写入字符串 */
void appendStr(RING_QUEUE *buffer, const char *str) {
    assert(str);
    size_t len = strlen(str);
    //printf("len:%d\n",len);
    append(buffer, str, len);
}

void freeBuffer(RING_QUEUE *buffer) {
    memset(buffer->data, 0, RING_QUEUE_SIZE);
    buffer->readPos = 0;
    buffer->writePos = 0;
}

/* 读取fd的内容写入到缓冲区 */
ssize_t BufferReadFd(int fd, RING_QUEUE *buffer, int* saveErrno) {
    char buff[65535];
    /* 用于分散和聚集io操作的数据结构 */
    struct iovec iov[2];
    const size_t writable = writableBytes(buffer);
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = BeginPtr(buffer) + buffer->writePos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    /* readv 一次性读取多个不连续内存块的内容 */
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    /* 如果缓冲区有足够的空间容纳fd的内容 */
    else if((size_t)len <= writable) {
        buffer->writePos += len;
    }
    /* 超出缓冲区可写区长度 */
    else {
        buffer->writePos = buffer->size;
        append(buffer, buff, len - writable);
    }
    return len;
}

/* 将缓冲区的数据写入fd */
ssize_t BufferWriteFd(int fd, RING_QUEUE *buffer, int* saveErrno) {
    size_t readSize = readableBytes(buffer);
    ssize_t len = write(fd, peek(buffer), readSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    buffer->readPos += len;
    return len;
}

#if 0
/* main函数只为进行简单的单元测试 */
int main(int argc, char argv[])
{
	RING_QUEUE ring_que = {0};
	ring_queue_init(&ring_que);
    char data[] = "it a test!";
    appendStr(&ring_que, data);
    printf("%s writePos:%d readPos:%d\n",ring_que.data, ring_que.writePos, ring_que.readPos);
    appendStr(&ring_que, "new world!\n");
    printf("%s writePos:%d readPos:%d\n",ring_que.data, ring_que.writePos, ring_que.readPos);
	return 0;
}
#endif


