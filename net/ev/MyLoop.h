//
//  myloop.h
//  MF
//
//  Created by mingweiliu on 17/1/22.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myev_h
#define myev_h

#include <thread>
#include <set>
#include "MyWatcher.h"
#include "util/MyQueue.h"

namespace MF {
    namespace EV {
        /// libev 循环，自动设置backend类型
        // loop本身不持有添加的 watcher，需要调用者自行管理生命周期
        class MyLoop {
        private:
#define lock_start std::lock_guard<std::mutex> guard(mutex_);
            typedef std::function<void()> Function;
        public:
            typedef struct ev_loop ev_loop_t;
            
            /**
             *  @brief 构造函数
             */
            explicit MyLoop(uint32_t flags) {
#ifdef __APPLE__
                loop_ = ev_loop_new(EVBACKEND_KQUEUE|flags);
#elif _UNIX
                loop_ = ev_loop_new(EVBACKEND_EPOLL|flags);
#else
                loop_ = ev_loop_new(flags); //其他平台
#endif
                //增加queue watcher
                auto func = [this] (MyWatcher*) {
                    //收到一次通知，执行所有的未执行的函数
                    while (true) {
                        Function f;
                        auto rv = this->queue_.popFront(f, 0); //每次等待0毫秒
                        if (rv && f) {
                            f();
                        } else {
                            break; //退出循环
                        }
                    }
                    
                    //判断是否需要退出
                    if (this->exit_) {
                        ev_break(this->loop_, EVBREAK_ALL);
                        return;
                    }
                    
                };
                queue_watcher_ = MyWatcherManager::GetInstance()->create<MyAsyncWatcher>(std::move(func));
                add(queue_watcher_);
                
                idle_watcher_ = MyWatcherManager::GetInstance()->create<MyIdleWatcher>([] (MyWatcher*) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); //休眠10毫秒
                });
                add(idle_watcher_);
            }
            
            /**
             *  @brief 析构函数
             */
            ~MyLoop() {
                MyWatcherManager::GetInstance()->destroy(queue_watcher_); //删除watcher
                MyWatcherManager::GetInstance()->destroy(idle_watcher_);
            }
            
            /**
             *  @brief 增加一个watcher
             *
             *  @param watcher 监听的事件
             *
             */
            void add(MyWatcher* watcher) {
                lock_start
                watcher->append(loop_);
            }
            
            /**
             *  @brief 删除一个事件
             *
             *  @param watcher 事件
             *
             */
            void remove(MyWatcher* watcher) { //TODO: 不太确定是否可以在外层这样删除一个事件的监听
                lock_start
                watcher->subtract();
            }
            
            /**
             *  @brief 执行循环
             *
             */
            void start() {
                while (!exit_) {
                    auto rv = ev_run(loop_, 0);
                    LOG(INFO) << "loop exit, rv: " << rv;
                    sleep(1);
                    if (rv == 0) { //如果没有事件了，那么就退出循环
                        break;
                    }
                }
            }
            
            /**
             *  @brief 退出循环
             */
            void stop() {
                lock_start
                exit_ = true;
                queue_watcher_->signal(); //发出通知
            }
            
            /**
             *  @brief 线程执行
             *
             *  @param pred 需要执行的函数
             *
             */
            template<typename Pred, class... Args>
            void RunInThread(Pred&& pred, Args... args) {
                //放进队列
                queue_.pushBack(std::bind(pred, args...));
                {
                    lock_start
                    //发出事件信号
                    queue_watcher_->signal();
                }
            }
            
            /**
             *  @brief 执行某个函数并且等待
             *
             *  @param pred    需要执行的函数
             *
             */
            template<typename Pred, class... Args>
            void RunInThreadAndWait(Pred&& pred, Args... args) {
                std::mutex m;
                std::condition_variable c;
                bool ready = false;
                auto func = [&m, &c, pred, &ready] () {
                    std::lock_guard<std::mutex> guard(m);
                    ready = true;
                    pred();
                    c.notify_one();
                };

                queue_.pushBack(std::bind(pred, args...)); //添加包裹之后的函数
                {
                    lock_start
                    queue_watcher_->signal(); //唤醒evloop
                }
                std::unique_lock<std::mutex> lock(m);
                c.wait(lock,[&ready]{return ready;});
            }
            
            /**
             *  @brief 等待N秒后，再执行
             *
             *  @param pred 需要执行的函数
             *  @param delay delay的时长(second)
             *
             */
            template<typename Pred, class... Args>
            void RunInThreadAfterDelay(Pred&& pred, uint32_t delay, Args... args) {
                //创建timer对象
                auto func = [pred](MyWatcher* watcher) {
                    //执行任务
                    pred(reinterpret_cast<MyTimerWatcher*>(watcher));
                    
                    //释放watcher
                    MyWatcherManager::GetInstance()->destroy(watcher);
                };
                //生成定时器
                add(MyWatcherManager::GetInstance()->create<MyTimerWatcher>(
                        std::move(std::bind(func, std::placeholders::_1, args...)), delay, 0));
            }
            
            /**
             *  @brief 等待N秒后，再执行，然后循环执行
             *
             *  @param pred 需要执行的函数
             *  @param delay delay的时长(second)
             *  @param repeat repeat的间隔(second)
             *
             */
            template<typename Pred, class... Args>
            void RunInThreadAfterDelayAndRepeat(Pred&& pred, uint32_t delay, uint32_t repeat, Args... args) {
                //创建timer对象
                auto func = [pred](MyWatcher* watcher) {
                    //执行任务
                    pred(reinterpret_cast<MyTimerWatcher*>(watcher));
                };

                //生成定时器
                add(MyWatcherManager::GetInstance()->create<MyTimerWatcher>(
                        std::move(std::bind(func, std::placeholders::_1, args...)), delay, repeat));
            }

        protected:
            MyLoop(MyLoop& r) = delete; //不允许拷贝
            MyLoop& operator= (MyLoop& r) = delete; //不允许赋值
        public:
            ev_loop_t* loop_; //循环
            MyThreadQueue<Function> queue_; //需要执行的任务队列
            MyAsyncWatcher* queue_watcher_; //队列的watcher
            MyIdleWatcher* idle_watcher_; //空闲的watcher
            bool exit_ {false}; //退出循环
            std::mutex mutex_;
        };
    }
}

#endif /* myev_h */
