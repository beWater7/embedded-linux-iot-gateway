#ifndef __PTHREAD_POOL_H__
#define __PTHREAD_POOL_H__

#include <pthread.h>

typedef struct task_t task_t;
typedef struct task_queue_t task_queue_t;
typedef struct thread_pool_t thread_pool_t;

/* 任务结构体 */
struct task_t {
  void (*func)(void* arg); // 函数指针
  void* arg;               // 传递给函数的参数
  task_t* next;            // 指向下一个任务的指针
};

/* 任务队列，将task组织起来的链表 */
struct task_queue_t {
  task_t* head;
  task_t* tail;
  pthread_mutex_t mutex;   // 互斥锁，用于保护任务队列的访问
  int size;                // 当前任务队列中任务的数量
  thread_pool_t *pool;      // 
};

/* 线程池结构体 */
struct thread_pool_t {
  pthread_t* threads;       // 线程数组
  int thread_count;         // 线程数量
  task_queue_t* queue;      // 指向任务队列的指针
  pthread_mutex_t mutex;    // 线程池的互斥锁
  pthread_cond_t cond;      // 条件变量，用于线程同步
  int running;              // 线程池运行状态
  int tasks_done;           // 已完成的任务数量
};

task_t* task_create(void (*func)(void* arg), void* arg);
void task_destroy(task_t* task);
task_queue_t* task_queue_create(thread_pool_t *pool);
void task_queue_destroy(task_queue_t* queue);
void task_queue_push(task_queue_t* queue, task_t* task);
task_t* task_queue_pop(task_queue_t* queue);
void* thread_func(void* arg);
int thread_pool_create(thread_pool_t* pool, int thread_count);
void thread_pool_destroy(thread_pool_t* pool);
 
#endif