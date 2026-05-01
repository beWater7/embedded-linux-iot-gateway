#ifndef __BUFFER_H__
#define __BUFFER_H__



#define ELEMENT char
#define RING_QUEUE_SIZE 4*1024  //4k

/*
 * |__|__|__|__|__|__|__|
 *       |     |
 *       r     w
 * r：读指针、便于从缓冲区读数据
 * w: 写指针、向缓冲区写数据
 * 基于C语言实现的环形缓冲区，但是无法自动增长
 * 自动增长的缓冲区优点是灵活，不需要事先预知内存需要多少，
 * 但是存在内存上的问题（需要malloc实现）
 */

typedef struct ring_queue{
   int front;     // 队头指针
   int rear;      // 队尾指针
   int readPos;   // 
   int writePos;  //
   int size;      // 队列长度
   ELEMENT data[RING_QUEUE_SIZE]; // 数组
}RING_QUEUE;


int ring_queue_init(RING_QUEUE *ring_que);
size_t readableBytes(const RING_QUEUE *buffer);
size_t writableBytes(const RING_QUEUE *buffer);
size_t prependableBytes(const RING_QUEUE*buffer);
const ELEMENT *peek(const RING_QUEUE *buffer);
void retrieve(RING_QUEUE *buffer, size_t len);
void retrieveUntil(RING_QUEUE *buffer, const char *end);
void retrieveAll(RING_QUEUE *buffer);
void retrieveAllToStr(RING_QUEUE *buffer, char *output);
const ELEMENT *beginWriteConst(const RING_QUEUE *buffer);
ELEMENT *beginWrite(RING_QUEUE *buffer);
void hasWritten(RING_QUEUE *buffer, size_t len);
ELEMENT * BeginPtr(RING_QUEUE *buffer);
void append(RING_QUEUE *buffer, const char *str, size_t len);
void appendStr(RING_QUEUE *buffer, const char *str);
void freeBuffer(RING_QUEUE *buffer);
ssize_t BufferReadFd(int fd, RING_QUEUE *buffer, int* saveErrno);
ssize_t BufferWriteFd(int fd, RING_QUEUE *buffer, int* saveErrno);   


#endif
