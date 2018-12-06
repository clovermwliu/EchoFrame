//
//  myserver.cc
//  MyFramewrok
//
//  Created by mingweiliu on 2017/2/17.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "net/server/MyServer.h"

namespace MF {
    namespace Server {
        int32_t MyServer::initServer(const MF::Server::ServerConfig &config) {
            this->config = config;

            //初始化loop manager
            this->loopManager = new EventLoopManager();
            return this->loopManager->initialize(this->config.ioThreadCount);
        }

        int32_t MyServer::startServer() {
            //1. 循环启动每一个服务
            for(auto it = servantMap.begin(); it != servantMap.end(); ++it) {
                if (it->second != nullptr) {
                    it->second->startServant();
                }
            }
            return 0;
        }

        void MyServer::wait() {
            while(!loopManager->wait()) {
                //检查是否需要注册节点
                for (auto it = servantMap.begin(); it != servantMap.end(); ++it){
                    if (it->second == nullptr
                        || config.routeServantName.empty()
                        || it->first == config.routeServantName) {
                        continue;
                    }

                    //检查是否需要发送注册
                    if (!it->second->isRegistered()) {
                        LOG(INFO) << "register servant, name: " << it->first << std::endl;
                        it->second->registerServant(config.routeServantName);
                    }

                    //检查是否需要发送心跳
                    if (MyTimeProvider::now() - it->second->getLastSyncTime() > 1) {
                        LOG(INFO) << "sync servant, name: " << it->first << std::endl;
                        it->second->syncServant(config.routeServantName);
                    }
                }

                //sleep1秒
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}
