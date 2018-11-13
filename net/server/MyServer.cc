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
            this->loopManager->wait();
        }
    }
}
