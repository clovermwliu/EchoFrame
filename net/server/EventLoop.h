//
// Created by mingweiliu on 2018/11/13.
//

#ifndef MYFRAMEWORK2_MYIOLOOP_H
#define MYFRAMEWORK2_MYIOLOOP_H

#include "net/MyGlobal.h"
#include "net/ev/MyLoop.h"
#include "net/server/MyChannel.h"
namespace MF {
    namespace Server {

        /**
         * IO循环
         */
        class EventLoop : public EV::MyLoop{
        public:
            EventLoop(uint32_t flags);

            virtual ~EventLoop();

            /**
             * 增加channel
             */
            void addChannel(std::shared_ptr<MyChannel> channel);

            /**
             * 删除channel
             * @param channel channel
             */
            void removeChannel(std::shared_ptr<MyChannel> channel);

            /**
             * 查找channel
             * @param uid uid
             * @return channel
             */
            std::shared_ptr<MyChannel> findChannel(uint32_t uid);

        protected:
            std::map<uint32_t , std::shared_ptr<MyChannel> > channels; //连接map
        };

        class EventLoopManager {
        public:
            //构造函数
            EventLoopManager() = default;

            //析构函数
            virtual ~EventLoopManager();

            /**
             *  @brief 初始化loop mananger
             *
             *  @param count loop的个数
             *
             */
            int32_t initialize(uint32_t count);

            /**
             *  @brief 根据index获取对应的loop
             *
             *  @param index 索引号
             *
             *  @return loop
             */
            EventLoop* get(uint32_t index);

            /**
             *  @brief 停止io线程
             */
            void stop() {
                for (auto it = loops_.begin(); it != loops_.end(); ++it) {
                    (*it)->stop();
                }
            }

            /**
             *  @brief 获取下一个循环
             *
             *  @return 获取到的循环
             */
            EventLoop* getNextLoop() {
                if (cur_index_ >= loops_.size()) {
                    cur_index_ = 0;
                }

                auto l = loops_[cur_index_];
                ++cur_index_;

                return l;
            }

            /**
             * 根据uid获取evloop
             * @param uid uid
             * @return loop
             */
            EventLoop* getByUid(uint32_t uid) {
                if (loops_.empty()) {
                    return nullptr;
                }

                return loops_[uid % loops_.size()];
            }

            /**
             * 根据uid获取对应的channel
             * @param uid uid
             * @return channel
             */
            std::shared_ptr<MyChannel> findChannel(uint32_t uid ) {
                auto lp = getByUid(uid);
                return lp == nullptr ? nullptr : lp->findChannel(uid);
            }

            /**
             * 增加一个channel
             * @param channel channel
             */
            void addChannel(std::shared_ptr<MyChannel> channel) {
                auto lp = getByUid(channel->getUid());
                if (lp == nullptr) {
                    return;
                }

                lp->addChannel(channel);
            }

            /**
             * 删除channel
             * @param channel channel
             */
            void removeChannel(std::shared_ptr<MyChannel> channel) {
                auto lp = getByUid(channel->getUid());
                if (lp == nullptr) {
                    return;
                }

                lp->removeChannel(channel);
            }

            /**
             *  @brief 等待io线程结束
             */
            void wait() {
                for (auto it = threads_.begin(); it != threads_.end(); ++it) {
                    it->join();
                }
            }
        protected:

            EventLoopManager(EventLoopManager& r) = delete;
            EventLoopManager& operator=(EventLoopManager& r) = delete;
        private:
            std::vector<EventLoop*> loops_; //所有的事件循环
            std::vector<std::thread> threads_; //所有的线程

            //用于等待线程启动完成
            std::mutex mutex_;
            std::condition_variable cond_;

            std::atomic<int32_t> cur_index_ {0}; //当前循环到的线程标识
        };
    }
}
#endif
