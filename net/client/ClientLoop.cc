//
// Created by mingweiliu on 2018/11/15.
//

#include "net/client/ClientLoop.h"
namespace MF {
    namespace Client{
        ClientLoop::ClientLoop(uint32_t flags) : MyLoop(flags){

        }

        ClientLoop::~ClientLoop() {
            clients.clear();
        }

        void ClientLoop::addClient(std::shared_ptr<MF::Client::MyClient> client) {
            clients[client->getUid()] = client;
        }

        std::shared_ptr<MyClient> ClientLoop::findClient(uint64_t uid) {
            return clients.find(uid) != clients.end() ? clients[uid] : nullptr;
        }

        std::shared_ptr<MyClient> ClientLoop::findClient(const std::string &host, uint16_t port) {
            std::shared_ptr<MyClient> client = nullptr;
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                auto c = it->second->getConfig();
                if (c.host == host && c.port == port) {
                    client = it->second;
                    break;
                }
            }

            return client;
        }

        const std::map<uint32_t , std::shared_ptr<MyClient>>& ClientLoop::getClientMap() const {
            return clients;
        }

        void ClientLoop::removeClient(std::shared_ptr<MF::Client::MyClient> client) {
            clients.erase(client->getUid());
        }

        void ClientLoop::onIdle() {
            //检查是否有链接断开了
        }

        ClientLoopManager::~ClientLoopManager() {
           for (auto it = loops_.begin(); it != loops_.end(); ++it) {
               (*it)->stop();
           }

           //等待所有线程结束
           for (auto it = threads_.begin(); it != threads_.end(); ++it) {
               it->join();
           }

           //释放所有的loop
           for (auto it = loops_.begin(); it != loops_.end(); ++it) {
               delete *it;
           }
       }
       int32_t ClientLoopManager::initialize(uint32_t count) {
           if (count == 0) { //系统自行决定
               return -1;
           }

           int rv = 0;
           for (auto i = 0; i < count; ++i) {
               ClientLoop* loop = new ClientLoop(EVFLAG_AUTO);
               std::thread t([loop]() {
                   loop->start();
               });

               std::unique_lock<std::mutex> lock(mutex_);
               if(!cond_.wait_for(lock, std::chrono::seconds(3), [&t]{return t.joinable();})) {
                  rv = -1;
                   break;
               }
               threads_.push_back(std::move(t));
               loops_.push_back(loop);
           }

           return 0;
       }

       ClientLoop* ClientLoopManager::get(uint32_t index) {
           if (loops_.size() <= index) {
               throw std::out_of_range("index out of range");
           }

           return loops_[index];
       }
    }
}
