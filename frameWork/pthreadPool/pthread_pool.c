/*************************************************************************
    > File Name: pthread_pool_v3.c
    > Author: liudayi
    > Mail: 1781405401@qq.com 
    > Created Time: 2023年05月18日 星期四 23时59分50秒
 ************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "pthread_pool.h"


/* 创建任务 */
task_t* task_create(void (*func)(void* arg), void* arg) {
  task_t* task = (task_t*)malloc(sizeof(task_t));
  task->func = func;
  task->arg = arg;
  task->next = NULL;
  return task;
}

/* 删除任务 */
void task_destroy(task_t* task) {
  free(task);
}

/* 初始化任务队列 */
task_queue_t* task_queue_create(thread_pool_t *pool) {
  task_queue_t* queue = (task_queue_t*)malloc(sizeof(task_queue_t));
  queue->head = NULL;
  queue->tail = NULL;
  queue->pool = pool;
  pthread_mutex_init(&queue->mutex, NULL);
  queue->size = 0;
  return queue;
}

/* 销毁任务队列 */
void task_queue_destroy(task_queue_t* queue) {
  pthread_mutex_lock(&queue->mutex);
  /* 遍历任务队列，将任务取出来 */
  while (queue->head) {
    task_t* task = queue->head;
    queue->head = task->next;
    task_destroy(task); // 销毁
  }
  queue->tail = NULL;
  pthread_mutex_unlock(&queue->mutex);
  pthread_mutex_destroy(&queue->mutex);
  free(queue);
}

/* 将任务加入到任务队列中 */
void task_queue_push(task_queue_t* queue, task_t* task) {
  pthread_mutex_lock(&queue->mutex);
  if (queue->tail) {
    queue->tail->next = task; // 尾插法1
  } else {
    queue->head = task;   // 任务队列为空，第一次插入
  }
  queue->tail = task; // 尾插法2
  queue->size++;
  pthread_mutex_unlock(&queue->mutex);

  // 通知等待的线程有新的任务可用
  //pthread_cond_signal(&queue->pool->cond);
}

/* 从任务队列中取出任务 */
task_t* task_queue_pop(task_queue_t* queue) {
  pthread_mutex_lock(&queue->mutex);
  task_t* task = queue->head;
  /* task 不为空，将其从任务队列中删除 */
  if (task) {
    queue->head = task->next;
    /* 队列为空时 */
    if (!queue->head) {
      queue->tail = NULL;
    }
    queue->size--;
  }
  pthread_mutex_unlock(&queue->mutex);
  return task;
}

/* 线程的执行函数，arg即为线程池结构体
 * 多个线程同时执行此函数，执行从工作队列中取出的任务
 */
void* thread_func(void* arg) {
  thread_pool_t* pool = (thread_pool_t*)arg;
  pthread_t tid = pthread_self();
  /* 当线程池处于运行状态时，
   * 从任务队列中取出任务，
   * 执行完就删除
   */
  while (pool->running) {
    task_t* task = task_queue_pop(pool->queue);
    if (task) {
      task->func(task->arg);
      task_destroy(task);

      // 已完成的任务数量加一
      pthread_mutex_lock(&pool->mutex);
      pool->tasks_done++;
      pthread_mutex_unlock(&pool->mutex);
    } else {
      pthread_mutex_lock(&pool->mutex);
      // 等待新任务的到来
      pthread_cond_wait(&pool->cond, &pool->mutex);
      pthread_mutex_unlock(&pool->mutex);
    }
  }
  return NULL;
}

/* 创建线程池 */
int thread_pool_create(thread_pool_t* pool, int thread_count) {
  pthread_attr_t attr;
    
  /* 初始化线程属性 */
  pthread_attr_init(&attr);

  /* 分配线程数组和任务队列 */
  pool->threads = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
  pool->thread_count = thread_count;
  pool->queue = task_queue_create(pool);

  /* 初始化互斥锁和条件变量 */
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->cond, NULL);

  /* 设置线程分离属性 */
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) 
  {
      perror("pthread_attr_setdetachstate");
      exit(EXIT_FAILURE);
  }

  /* 创建线程，创建了 thread_count 个子线程 */
  for (int i = 0; i < thread_count; ++i) {
    pthread_create(&pool->threads[i], &attr, thread_func, pool);
  }

  pool->running = 1;
  pool->tasks_done = 0;
  return 0;
}

/* 销毁线程池 */
void thread_pool_destroy(thread_pool_t* pool) {
  // 停止线程池运行
  pool->running = 0;

  // 唤醒所有线程并等待它们退出
  pthread_cond_broadcast(&pool->cond);

  for (int i = 0; i < pool->thread_count; ++i) {
    pthread_join(pool->threads[i], NULL);
  }

  // 销毁互斥锁和条件变量
  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->cond);

  // 销毁任务队列
  task_queue_destroy(pool->queue);

  // 释放线程数组的内存
  free(pool->threads);
}

#if 0
/* 示例任务函数 */
void task_func(void* arg) {
  int task_id = *((int*)arg);
  printf("Task %d is running\n", task_id);
}

int main() {
  thread_pool_t pool;
  thread_pool_create(&pool, 4);

  // 创建10个示例任务
  for (int i = 0; i < 10; ++i) {
    int* arg = (int*)malloc(sizeof(int));
    *arg = i;
    task_t* task = task_create(task_func, arg);
    task_queue_push(pool.queue, task);
  }

  // 等待所有任务完成
  while (pool.tasks_done < 10) {
    usleep(100);
  }

  thread_pool_destroy(&pool);
  return 0;
}
#endif
