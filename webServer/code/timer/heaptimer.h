/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

 
/* 定义了一个没有参数和返回值的函数类型别名 */
typedef std::function<void()> TimeoutCallBack;
/* 高精度时钟 */
typedef std::chrono::high_resolution_clock Clock;
/* 表示毫秒的时间单位 */
typedef std::chrono::milliseconds MS;
/* 即时钟的时刻 */
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    /* const TimerNode& t 是一个函数参数，
       表示要与当前对象进行比较的另一个 TimerNode 对象。
       这个参数是通过成员函数调用的方式传递进来的。
       在结构体内对 < 进行了重载，如node1 < node2
       将直接比较两者的expires
       */
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
public:
    /* 容器预分配空间, 至少容纳64个元素 */
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    /* 调整指定ID节点的超时时间，并进行堆调整。*/
    void adjust(int id, int newExpires);
    /* 添加定时器节点。如果ID不存在，则插入新节点并进行堆调整；如果ID已存在，则更新对应节点的超时时间，并进行堆调整。*/
    void add(int id, int timeOut, const TimeoutCallBack& cb);
    /* 根据ID执行定时器节点的回调函数，并删除对应节点 */
    void doWork(int id);
    /* 清空定时器。*/
    void clear();
    /* 处理所有已过期的定时器节点，执行它们的回调函数，并从堆中移除。*/
    void tick();
    /*从堆中移除堆顶节点，即最小的超时节点。*/
    void pop();
    /* 获取距离下一个超时的时间（以毫秒为单位），如果没有定时器节点，则返回 -1。*/
    int GetNextTick();

private:
    /* 删除堆中指定位置的节点，并进行堆调整。*/
    void del_(size_t i);
    /* 执行堆的向上调整操作，确保新插入的节点满足堆的性质。 */
    void siftup_(size_t i);
    /* 执行堆的向下调整操作，确保删除或修改节点后，堆仍然满足性质。 */
    bool siftdown_(size_t index, size_t n);
    /*交换堆中两个节点的位置，并更新节点在 ref_ 中的索引。*/
    void SwapNode_(size_t i, size_t j);

    /* 用于存储定时器节点的堆，每个节点包含一个ID、过期时间以及回调函数。*/
    std::vector<TimerNode> heap_;
    /* 用于将ID映射到堆中对应节点的索引位置。 */
    std::unordered_map<int, size_t> ref_;
};

#endif //HEAP_TIMER_H