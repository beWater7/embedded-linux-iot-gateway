/*
 * @Author       : mark
 * @Date         : 2020-06-15
 * @copyleft Apache 2.0
 */ 

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
public:
    /* 指定该构造函数不能隐式地被用于类型转换。具体来说，如果一个构造函数被声明为 explicit，那么它不能被用于隐式地将一个类型转换为另一个类型
     * (size_t threadCount = 8) 构造函数的参数列表、如果定义对象时没指定该参数，则默认为8
     * pool_(std::make_shared<Pool>()): 初始化ThreadPool的成员变量pool_
     * std::make_shared<Pool>() 此函数在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr；
     */
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);
            for(size_t i = 0; i < threadCount; i++) {
                /* 创建一个新线程并运行指定的可调用对象 
                 * std::thread 的构造函数接受一个可调用对象，这里使用了一个 Lambda 表达式，定义了线程的执行逻辑。
                 * [pool = pool_]： 这是捕获列表，用于将 pool_ 捕获到 Lambda 表达式中。通过使用 =，我们进行值捕获，
                 * 并且使用 pool = pool_ 语法，将外部变量 pool_ 的值复制到名为 pool 的 Lambda 内部变量中。
                 * 这样确保在 Lambda 表达式的生命周期内使用的是线程创建时的 pool_ 值。
                 * std::unique_lock<std::mutex> locker(pool->mtx); std::unique_lock：模板类，用来管理多种互斥量 
                 *      <std::mutex> 指定了互斥量的类型、locker：对象，锁定的对象pool->mtx，这种方式又称为构造时上锁
                 * 
                 */
                std::thread([pool = pool_] {
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while(true) {
                        if(!pool->tasks.empty()) {
                            auto task = std::move(pool->tasks.front()); // 取出第一个任务
                            pool->tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } 
                        else if(pool->isClosed) break;  //线程池关闭
                        else pool->cond.wait(locker);   //线程池为关闭，任务队列为空，等待信号通知
                    }
                }).detach(); //线程设置为detach，不需要显式的释放资源
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() {
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            /* 通过notify_all唤醒所有在等待的线程 */
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            /* emplace 是一个成员函数，用于在 tasks 队列的末尾就地构造一个新元素 
             * std::forward<F>(task)：这里使用了 std::forward 来进行完美转发（perfect forwarding）。
             * F 是一个模板参数，表示任务的类型。task 是传递给函数的任务对象。
             * std::forward 会根据 task 的类型（是左值还是右值）进行适当的转发，以保留原始对象的左值或右值属性。
             *
             */
            pool_->tasks.emplace(std::forward<F>(task));
        }
        /* 唤醒在等待的一个线程 */
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks; //无返回值和参数的task队列
    };
    /* shared_ptr：智能指针，多个ptr能指向同一个个对象 */
    std::shared_ptr<Pool> pool_;
};

/*
std::lock_guard 是一种轻量级的互斥量封装，它提供了自动的构造和析构，且在构造时锁定互斥量，在析构时解锁互斥量。但是，std::lock_guard 没有提供手动锁定和解锁的功能。
std::unique_lock 是更灵活的工具，它提供了手动锁定和解锁互斥量的能力。你可以选择在构造时锁定互斥量，也可以在后续的代码中手动锁定和解锁。开销大于上一种锁
*/

#endif //THREADPOOL_H