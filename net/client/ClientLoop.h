//
// Created by mingweiliu on 2018/11/15.
//

#ifndef MYFRAMEWORK2_CLIENTLOOP_H
#define MYFRAMEWORK2_CLIENTLOOP_H

#include "net/ev/MyLoop.h"
#include "net/client/MyClient.h"

namespace MF{
    namespace Client{
        /**
         * client的事件循环线程
         */
        class ClientLoop : public EV::MyLoop{

        public:
            /**
             * 构造函数
             * @param flags
             */
            ClientLoop(uint32_t flags);

            /**
             * 析构函数
             */
            virtual ~ClientLoop();

            /**
             * 增加一个proxy
             * @param proxy proxy
             */
            void addClient(std::shared_ptr<MyClient> client);

            /**
             * 根据uid 获取client
             * @param uid
             * @return client 对象
             */
            std::shared_ptr<MyClient> findClient(uint32_t uid);

            /**
             * 根据host 和 port查找client
             * @param host host
             * @param port port
             * @return client
             */
            std::shared_ptr<MyClient> findClient(const std::string& host, uint16_t port);

            /**
             * 获取client map
             * @return client map
             */
            const std::map<uint32_t , std::shared_ptr<MyClient>>& getClientMap() const;

            /**
             * 删除client
             * @param client client
             */
            void removeClient(std::shared_ptr<MyClient> client);

        protected:
            //map<servantName, proxy>
            std::map<uint32_t , std::shared_ptr<MyClient>> clients; //所有的proxy
        };

        class ClientLoopManager {
        public:
            //构造函数
            ClientLoopManager() = default;

            //析构函数
            virtual ~ClientLoopManager();

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
            ClientLoop* get(uint32_t index);

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
            ClientLoop* getNextLoop() {
                if (cur_index_ >= loops_.size()) {
                    cur_index_ = 0;
                }

                auto l = loops_[cur_index_];
                ++cur_index_;

                return l;
            }

            /**
             * 根据Client 的uid 分配线程
             * @param servantId uid
             * @return loop
             */
            ClientLoop* getByServantId(uint32_t servantId) {
                if (loops_.empty()) {
                    return nullptr;
                }

                return loops_[servantId % loops_.size()];
            }

            /**
            * 增加一个proxy
            * @param proxy proxy
            */
            void addClient(std::shared_ptr<MyClient> client) {
                auto lp = getByServantId(client->getServantId());
                if (lp != nullptr) {
                    lp->addClient(client);
                }
            }

            /**
             * 根据uid 获取client
             * @param uid
             * @return client 对象
             */
            std::shared_ptr<MyClient> findClient(uint32_t uid) {
                auto lp = getByServantId(uid & 0xFFFF);
                return lp == nullptr ? nullptr : lp->findClient(uid);
            }

            /**
             * 删除client
             * @param client client
             */
            void removeClient(std::shared_ptr<MyClient> client) {
                auto lp = getByServantId(client->getServantId());
                if (lp != nullptr) {
                    lp->removeClient(client);
                }
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

            ClientLoopManager(ClientLoopManager& r) = delete;
            ClientLoopManager& operator=(ClientLoopManager& r) = delete;
        private:
            std::vector<ClientLoop*> loops_; //所有的事件循环
            std::vector<std::thread> threads_; //所有的线程

            //用于等待线程启动完成
            std::mutex mutex_;
            std::condition_variable cond_;

            std::atomic<int32_t> cur_index_ {0}; //当前循环到的线程标识

        };
    }
}


#endif //MYFRAMEWORK2_CLIENTLOOP_H
