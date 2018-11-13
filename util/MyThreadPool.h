#ifndef __MYTHREADPOOL_H_
#define __MYTHREADPOOL_H_

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <memory>
#include <future>

#include "MyQueue.h"
#include "MyTimeoutMap.h"
using namespace std;

namespace MF
{
    //线程池类
    template<typename R>
    class MyThreadPool
    {
    public:
        
        typedef std::lock_guard<std::mutex> LockGuard;

        //job function
        using JobFunc = std::packaged_task<R ()>;

        /**
         * @brief 构造函数
         *
         */
        MyThreadPool()  = default;

        /**
         * @brief 析构, 会停止所有线程
         */
        ~MyThreadPool() {
            stop();
        }

        /**
         * @brief 初始化. 
         *  
         * @param num 工作线程个数
         *
         * @return true  成功  false 失败
         */
        bool initialize(size_t num) {
            //1. 停止上次的线程
            stop();

            //2. 清理线程池
            LockGuard guard(mutex_);
            clear();

            //3. 启动线程
            for(size_t i = 0; i < num; i++) {
                std::unique_ptr<MyThread> thread(new MyThread(this));
                workers_.push_back(std::move(thread));
            }

            return true;
        }

        /**
         * @brief 获取线程个数.
         *
         * @return size_t 线程个数
         */
        size_t GetThreadNum() {
            return workers_.size();
        }

        /**
         * @brief 获取线程池的任务数(exec添加进去的).
         *
         * @return size_t 线程池的任务数
         */
        size_t GetJobNum() {
            return job_queue_.size();
        }

        /**
         * @brief 停止所有线程
         */
        void stop() {
            //停止所有线程
            LockGuard guard(mutex_);

            //这里不能对线程做join操作，worker线程要等到所有任务都处理完成之后才退出
            for (auto it = workers_.begin(); it != workers_.end(); ++it) {
                (*it)->terminate();
            }
        }
        
        /**
         *  @brief 等待所有任务完成后退出
         */
        void StopAfterAllDone() {
            LockGuard guard(mutex_);
            //这里不能对线程做join操作，worker线程要等到所有任务都处理完成之后才退出
            for (auto it = workers_.begin(); it != workers_.end(); ++it) {
                (*it)->TerminateAfterAllDone();
            }
        }

        /**
         * @brief 启动所有线程
         */
        void start() {}

        /**
         * @brief 添加对象到线程池执行，该函数马上返回， 
         *  	  线程池的线程执行对象
         */
        void exec(std::shared_ptr<JobFunc> pred) { //使用右值引用
            job_queue_.pushBack(pred);
        }


        /**
         * @brief 等待线程池退出(任务队列未空，并且没有繁忙线程).
         *
         */
        void wait() {
            //1. 先等待所有线程都退出
            for (auto it = workers_.begin(); it != workers_.end(); ++it) {
                (*it)->join();
            }
            workers_.clear(); //清理线程队列
            job_queue_.clear(); //清理任务队列
        }
        
    protected:
        //工作线程
        class MyThread {
        public:
            /**
             *  @brief 构造函数
             */
            MyThread(MyThreadPool* pool) {
                exit_ = false;
                pool_ = pool;
                //启动线程
                std::thread t([this] () {
                    this->run();
                });

                thread_.swap(t);
            }
            
            /**
             *  @brief 析构函数
             */
            ~MyThread() {
                exit_ = true;
            }
            
            /**
             *  @brief 停止一个线程
             */
            void terminate() {
                exit_ = true;
            }
            
            /**
             *  @brief 等到所有任务执行完成后停止
             */
            void TerminateAfterAllDone() {
                exit_ = true;
                exit_after_done_ = true;
            }
            
            /**
             *  @brief 线程的执行函数
             */
            void run() {
                is_alive_ = true;
                while (true) { //持续的循环

                    if (exit_) {
                        if ((exit_after_done_ && pool_->GetJobNum() == 0)
                            || !exit_after_done_) { //如果完成了所有任务或者未设置该标记时，线程退出
                            break;
                        }
                    }

                    auto job = pool_->GetJob();
                    if (job != nullptr && job->valid()) { //job有效，执行该任务
                        (*(job.get()))();
                        pool_->idle(); //任务执行完了，线程空闲了
                    }
                }
                is_alive_ = false;
            }
            
            /**
             *  @brief 获取id信息
             *
             *  @return 线程id
             */
            std::thread::id GetId() const {
                return thread_.get_id();
            }
            
            /**
             *  @brief 判断线程是否活着
             *
             *  @return true 活着， false 死去
             */
            bool IsAlive() const {
                return is_alive_;
            }
            
            /**
             *  @brief 等待线程退出
             */
            void join() {
                thread_.join();
            }
        protected:
        private:
            MyThreadPool* pool_;
            bool exit_ {false}; //退出标志 true 退出 false 继续
            bool exit_after_done_ {false}; //线程执行完所有任务再退出
            std::thread thread_; //线程
            bool is_alive_ {false}; //线程是否存活
        };

    protected:

        /**
         * @brief 清除
         */
          void clear() {
            //1. 清理任务队列
            job_queue_.clear();
          }

        /**
         * @brief 获取任务, 如果没有任务, 则为NULL.
         *
         * @return 任务
         */
         std::shared_ptr<JobFunc> GetJob() {
            //2. 获取对应的任务
            std::shared_ptr<JobFunc> job = nullptr;
            if (job_queue_.popFront(job, 500)) { //最多等待500ms
            }

            return job;
         }

        /**
         * @brief 空闲了一个线程, 可以做点其他事情
         */
          void idle() {

          }
    protected:
        
        std::vector<std::unique_ptr<MyThread>> workers_; //所有的线程
        std::mutex mutex_; //线程锁
        std::condition_variable cond_; //条件变量
        
        MyThreadQueue<std::shared_ptr<JobFunc>> job_queue_; //任务队列
    };

    template<typename R>
    class MyThreadExecutor {
    public:
        /**
         *  @brief 构造函数
         */
        MyThreadExecutor(uint32_t count)  {
            if(!pool_.initialize(count)) {
                throw MyException("initialize executor thread pool");
            }
        }
        
        /**
         *  @brief 析构函数
         */
        virtual ~MyThreadExecutor() {
            pool_.stop();
        }
        

        /**
         * 添加到线程池，并且返回一个future对象
         * @param pred 需要执行的内容
         * @return
         */
        template<typename Predicate>
        std::future<int32_t > exec(Predicate&& pred) {
            auto task = std::make_shared<std::packaged_task<R()>>(pred);
            pool_.exec(task);
            return task->get_future();
        }
    private:
        MyThreadPool<R> pool_; //线程池
    };
}
#endif

