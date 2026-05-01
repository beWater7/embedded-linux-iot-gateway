#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <stdbool.h>
#include "heapTimer.h"

/* ：一种完全二叉树数据结构
 *大根堆示意图：
                     16
                   /    \
                 14      10
                /  \     / \
               8    7   9   3
              / \
             2   4
  大根堆在数组中排序  [16, 14, 10, 8, 7, 9, 3, 2, 4]
  (二叉树层次遍历的结果即数组排序)

  大根堆中的每一个节点都大于其子节点
  大根堆的存储通常是通过数组来实现的。在数组中，如果一个节点的索引为 i，
  则它的左子节点的索引是 2i + 1，右子节点的索引是 2i + 2，父节点的索引是 (i-1)/2。
  这样，数组的第一个元素（索引为0）就是堆的根。  


* 小根堆示意图：
                      3
                    /   \
                  4      5
                 / \    / \
                9   8  7   10
               / \
              14  16
   小根堆中的每一个节点都小于其子节点
   
   二叉堆通常使用数组来存储元素、index表示数组下标
   使用小根堆来管理定时事件的优势：
   1、因此最小的定时器（即最早触发的定时器）就是堆的根节点。这使得检索最小定时器的操作非常高效，只需要访问堆的根节点。
   2、插入和删除操作高效
   3、时间复杂度稳定
   4、堆的结构适用于优先级队列
   5、自然支持定时器的取消
*/

static size_t gs_ref[64];  //用于记录id和数组index的映射数组、即gs_ref[id] ==> index 

long long getCurrentTime()
{
    struct timeval te;
    gettimeofday(&te, NULL); // 获取当前时间
    long long currentTs = te.tv_sec * 1000LL + te.tv_usec / 1000; // 转换为毫秒
    return currentTs;
}


// 初始化小根堆
MaxHeap* initMaxHeap(int capacity) {
    MaxHeap* heap = (MaxHeap*)malloc(sizeof(MaxHeap));
    heap->array = (TimerNode*)malloc(sizeof(TimerNode) * capacity);
    heap->capacity = capacity;
    heap->size = 0;
    return heap;
}

// 交换两个元素的值
static void swap(TimerNode* a, TimerNode* b) {
    TimerNode temp = *a;
    *a = *b;
    *b = temp;
}

/* 根据节点索引交换两个元素 */
void SwapNode(MaxHeap* heap, size_t i, size_t j) {
    assert(i >= 0 && i < heap->capacity);
    assert(j >= 0 && j < heap->capacity);
    swap(&heap->array[i], &heap->array[j]);
    /* 更新索引值 */
    gs_ref[heap->array[i].id] = i;
    gs_ref[heap->array[j].id] = j;
} 


// 和父节点比较，维持小根堆性质
void heapSiftUp(MaxHeap* heap, size_t i) {
    assert(i >= 0 && i < heap->capacity);
    /* 找到当前节点的父节点 */
    int j = (i - 1) / 2;
    while( j >= 0 )
    {
        /* 如果父节点的的值小于当前节点，do noting */
        if(heap->array[j].expires < heap->array[i].expires) 
        { 
            break; 
        }

        /* 交换节点的位置 */
        SwapNode(heap, i, j);

        /* 当前节点更新为父节点*/
        i = j;
        j = (i - 1) / 2;
    }
}

// 和子节点比较，维持小根堆性质
bool heapSiftDown(MaxHeap* heap, size_t index, size_t n) {
    assert(index >= 0 && index < heap->capacity);
    assert(n >= 0 && n <= heap->capacity);
    size_t i = index;
    /* 找到当前节点的左子节点 */
    size_t j = i * 2 + 1; 
    while(j < n) {
        /* 右子节点 < n, 如果左子节点 大于 右子节点，则切换到到右子节点 (父节点要小于最小的子节点) */
        if(j + 1 < n && heap->array[j + 1].expires < heap->array[j].expires) j++;
       
        /* 如果当前节点小于子节点，do nothing */
        if(heap->array[i].expires < heap->array[j].expires) break;

        SwapNode(heap, i, j);

        /* 切换到子节点 */
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

// 插入一个定时器事件到小根堆
void heapInsert(MaxHeap* heap, int id, long long timeout, TimeoutCallBack cb) {

    assert(id >= 0);
    size_t i;
    if(gs_ref[id] == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap->size;
        gs_ref[id] = i; // 数组下标index个参数在 Linux 2.6.8 之后已经被忽略，因为内核会自动调整以适应需要的大小
        
        /* 插入新节点 */
        heap->array[i].id = id;
        heap->array[i].expires = getCurrentTime() + timeout; 
        heap->array[i].cb = cb;
        heapSiftUp(heap,i);
        heap->size++;
    } 
    else {
        /* 已有结点：调整堆 */
        i = gs_ref[id];
        heap->array[i].expires = getCurrentTime() + timeout; 
        heap->array[i].cb = cb;
        if(!heapSiftDown(heap, i, heap->size)) {
            heapSiftUp(heap, i);
        }
    }
}

void heapInsertNode(MaxHeap* heap, TimerNode *node) {
    heapInsert(heap, node->id, node->expires, node->cb);
}


/* 删除heap数组最后一个元素 */
void heapPopBack(MaxHeap *heap) {
    if(heap->size > 0)
        heap->size--;
}

void heapDel(MaxHeap *heap, size_t index) {
    /* 删除指定位置的结点 */
    assert(heap->size > 0 && index >= 0 && index < heap->capacity);
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap->size - 1;
    assert(i <= n);
    if(i < n) {
        /* 将节点i 和 数组最后一个元素交换 */
        SwapNode(heap, i, n);
        
        /* 进行堆调整 */
        if(!heapSiftDown(heap, i, n)) {
            heapSiftUp(heap, i);
        }
    }
    /* 队尾元素删除 */
    //ref_.erase(heap_.back().id);
    //heap_.pop_back();
    gs_ref[heap->array[heap->size].id] = -1;
    heapPopBack(heap);
}


void HeapAdjust(MaxHeap *heap, int id, long long timeout) {

    /* 调整指定id的结点 */
    assert(heap->size > 0 && gs_ref[id] > 0);
    heap->array[gs_ref[id]].expires = getCurrentTime() + timeout;;
    heapSiftDown(heap, gs_ref[id], heap->size);
}


void heapDoWork(MaxHeap* heap, int id) {
    /* 删除指定id结点，并触发回调函数 */
    if( 0 == heap->size || gs_ref[id] == 0) {
        return;
    }
    size_t i = gs_ref[id];
    heap->array[id].cb();
    heapDel(heap, i);
}


void heapPop(MaxHeap *heap) {
    assert(heap->size > 0);
    /* 移除根节点 */
    heapDel(heap, 0);
}


void heapTick(MaxHeap *heap) {

    /* 清除超时结点 */
    if(0 == heap->size) {
        return;
    }
    while(heap->size > 0) {
        if(heap->array[0].expires - getCurrentTime()> 0) { 
            break; 
        }
        /* 到达超时事件，调用回调函数 */
        heap->array[0].cb();
        /* 移除最小的那个节点（已经触发）*/
        heapPop(heap);
    }
}


void heapClear(MaxHeap *heap) {
    assert(heap);
    free(heap->array);
    free(heap);
}

/*返回距离最近的超时事件还有多久*/
int heapGetNextTick(MaxHeap *heap) {
    size_t res = -1;
    if(heap->size > 0) {
        res = heap->array[0].expires - getCurrentTime();
        if(res < 0) { res = 0; }
    }
    return res;
}



// 打印大根堆的元素
void printHeap(MaxHeap* heap) {
    printf("Heap: ");
    for (int i = 0; i < heap->size; i++) {
        printf("(%d, %lld) ", heap->array[i].id, heap->array[i].expires - getCurrentTime());
    }
    printf("\n");
}


#if 1
void func1()
{
    printf("1\n");
}

void func2()
{
    printf("2\n");
}

void func3()
{
    printf("3\n");
}

void func4()
{
    printf("4\n");
}

void func5()
{
    printf("5\n");
}



// 主函数 for test
int main() {

    // 初始化小根堆
    MaxHeap* heap = NULL;
    heap = initMaxHeap(16);

    TimerNode event1 = {0};
    TimerNode event2 = {0};
    TimerNode event3 = {0};
    TimerNode event4 = {0};
    TimerNode event5 = {0};

    event1.id = 0;
    event2.id = 1;
    event3.id = 2;
    event4.id = 3;
    event5.id = 4;

    event1.expires = 2000;     // ms之后触发
    event2.expires = 3000;     // ms之后触发
    event3.expires = 4000;     // ms之后触发
    event4.expires = 5000;     // ms之后触发
    event5.expires = 500;     // ms之后触发


    event1.cb = func1;
    event2.cb = func2;
    event3.cb = func3;
    event4.cb = func4;
    event5.cb = func5;
    
    // 插入一些定时器事件
    heapInsertNode(heap, &event1);
    heapInsertNode(heap, &event2);
    heapInsertNode(heap, &event3);
    heapInsertNode(heap, &event4);
    heapInsertNode(heap, &event5);

    // 打印堆中的元素
    printHeap(heap);

    while(1)
    {
        heapTick(heap);
        usleep(100000); //100ms
        printHeap(heap);
        if( 0 == heap->size)
            break;
    }

    heapClear(heap);
    return 0;
}
#endif
