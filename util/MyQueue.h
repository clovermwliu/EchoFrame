#ifndef __MYTHREADQUEUE_H_
#define __MYTHREADQUEUE_H_

#include <deque>
#include <vector>
#include <cassert>
#include <mutex>
#include <condition_variable>

namespace MF
{
    //线程安全队列, 支持使用其他类型
    template<typename T, typename D = std::deque<T> >
    class MyThreadQueue
    {

    public:

        typedef D QueueType;

        /**
         *  @brief 从头部获取数据
         *
         *  @param t          弹出的数据
         *  @param millsecond 等待的时长, 0 不等待, -1 永久等待
         *
         *  @return true 成功 false 失败
         */
        bool popFront(T &t, uint32_t millsecond = -1);

        /**
         * @brief 放数据到队列后端. 
         *  
         * @param t 对象
         */
        void pushBack(const T &t);

        /**
         * @brief  将队列放数据到队列后端.
         *  
         * @param qt 对象
         */
        void pushBack(const QueueType &qt);
        
        /**
         * @brief  放数据到队列前端. 
         *  
         * @param t 对象
         */
        void pushFront(const T &t);

        /**
         * @brief  放数据到队列前端. 
         *  
         * @param qt 新的队列
         */
        void pushFront(const QueueType &qt);

        /**
         * @brief  队列大小.
         *
         * @return size_t 队列大小
         */
        size_t size();

        /**
         * @brief  清空队列
         */
        void clear();

        /**
         * @brief  是否数据为空.
         *
         * @return bool 为空返回true，否则返回false
         */
        bool empty();
    
        typedef std::unique_lock<std::mutex> UniqueLock;
        
        /**
         *  @brief 通知一个线程醒过来
         */
        void notify();
        
        /**
         *  @brief 通知所有线程醒过来
         */
        void notifyAll();

    protected:
        QueueType queue_; //队列
        size_t size_ {0}; //队列长度
        
        std::mutex mutex_; //带超时的mutex
        std::condition_variable cond_; //cond

    };

    template<typename T, typename D> bool MyThreadQueue<T, D>::popFront(T &t, uint32_t millsecond) {
        
        UniqueLock lock(mutex_);
        bool rv = true;
        if (millsecond == -1) { //不等待
            if (queue_.empty()) {
                cond_.wait(lock);
            }
        } else if (millsecond == 0) {//不等待
            if (queue_.empty()) {
                return false;
            }
        } else {
            if (queue_.empty()) {
                rv = cond_.wait_for(lock, std::chrono::milliseconds(millsecond)) == std::cv_status::no_timeout;
            }
        }
        
        if (rv) {
            if (queue_.empty()) {
                return false;
            }
            t = queue_.front();
            queue_.pop_front();
        }
        return  rv;
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::pushBack(const T &t) {
        UniqueLock lock(mutex_);

        queue_.push_back(t);
        
        cond_.notify_one();
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::pushBack(const QueueType &qt) {

        UniqueLock lock(mutex_);
        
        typename QueueType::const_iterator it = qt.begin();
        typename QueueType::const_iterator itEnd = qt.end();
        while(it != itEnd) {
            queue_.push_back(*it);
            ++it;
            cond_.notify_one();
        }
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::pushFront(const T &t) {

        UniqueLock lock(mutex_);

        queue_.push_front(t);
        
        cond_.notify_one();
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::pushFront(const QueueType &qt) {
        UniqueLock lock(mutex_);

        typename QueueType::const_iterator it = qt.begin();
        typename QueueType::const_iterator itEnd = qt.end();
        while(it != itEnd)
        {
            queue_.push_front(*it);
            ++it;

            cond_.notify_one();
        }
    }

    template<typename T, typename D> size_t MyThreadQueue<T, D>::size() {
        UniqueLock lock(mutex_);
        
        return queue_.size();
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::clear() {
        UniqueLock lock(mutex_);
        queue_.clear();
    }

    template<typename T, typename D> bool MyThreadQueue<T, D>::empty() {
        UniqueLock lock(mutex_);
        return queue_.empty();
    }
    
    template<typename T, typename D> void MyThreadQueue<T, D>::notify() {
        UniqueLock lock(mutex_);
        cond_.notify_one();
    }

    template<typename T, typename D> void MyThreadQueue<T, D>::notifyAll() {
        UniqueLock lock(mutex_);
        cond_.notify_all();
    }
}
#endif

