//
// Created by mingweiliu on 2018/11/13.
//

#include "net/server/EventLoop.h"

namespace MF {
    namespace Server {
        EventLoopManager::~EventLoopManager() {
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

        int32_t EventLoopManager::initialize(uint32_t count) {
            if (count == 0) { //系统自行决定
                return -1;
            }

            int rv = 0;
            for (auto i = 0; i < count; ++i) {
                EventLoop* loop = new EventLoop(EVFLAG_AUTO);
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

        EventLoop* EventLoopManager::get(uint32_t index) {
            if (loops_.size() <= index) {
                throw std::out_of_range("index out of range");
            }

            return loops_[index];
        }

        EventLoop::EventLoop(uint32_t flags) : MyLoop(flags) {}

        EventLoop::~EventLoop() {
            channels.clear();
        }

        void EventLoop::addChannel(std::shared_ptr<MF::Server::MyChannel> channel) {
            channels[channel->getUid()] = channel;
        }

        void EventLoop::removeChannel(std::shared_ptr<MF::Server::MyChannel> channel) {
            channel->close(); //关闭channel
            channels.erase(channel->getUid());
        }

        std::shared_ptr<MyChannel> EventLoop::findChannel(uint32_t uid) {
            return channels.find(uid) != channels.end() ? channels[uid] : nullptr;
        }
    }
}
