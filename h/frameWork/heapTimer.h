#ifndef __HEAP_TIMER_H__
#define __HEAP_TIMER_H__

#include <time.h>

typedef void (* TimeoutCallBack)(void);
#define MAX_TIME_EVENT 64

// 定时器事件的结构体
typedef struct {
    int id;         // 定时器标识符\ 使用fd
    long long expires; // 触发时间
    TimeoutCallBack cb;
    // 其他定时器相关的信息可以添加在这里
}TimerNode;

// 大根堆的结构体
typedef struct {
    TimerNode *array; // 存储定时器事件的数组，此处使用指针的原因是C语言不支持可变长度的数组
    int capacity;      // 堆的容量
    int size;          // 堆的当前大小
} MaxHeap;


long long getCurrentTime();
MaxHeap* initMaxHeap(int capacity);

void SwapNode(MaxHeap* heap, size_t i, size_t j); 
void heapSiftUp(MaxHeap* heap, size_t i);
bool heapSiftDown(MaxHeap* heap, size_t index, size_t n);
void heapInsert(MaxHeap* heap, int id, long long timeout, TimeoutCallBack cb);
void heapInsertNode(MaxHeap* heap, TimerNode *node);
void heapPopBack(MaxHeap *heap);
void heapDel(MaxHeap *heap, size_t index);
void HeapAdjust(MaxHeap *heap, int id, long long timeout);
void heapDoWork(MaxHeap* heap, int id);
void heapPop(MaxHeap *heap);
void heapTick(MaxHeap *heap);
void heapClear(MaxHeap *heap);
int heapGetNextTick(MaxHeap *heap);  
void printHeap(MaxHeap* heap);
#endif
