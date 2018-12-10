//
// Created by mingweiliu on 2018/12/7.
//

#ifndef MYFRAMEWORK2_MYAPPLICATION_H
#define MYFRAMEWORK2_MYAPPLICATION_H

#include "net/server/MyServer.h"
#include "net/client/MyCommunicator.h"

namespace MF {
    namespace Application {
        /**
         * Application类，具有以下功能
         * 1. 解析配置文件
         * 2. 管理Server配置文件和更新
         * 3. 管理Client配置文件和更新
         */
        class MyApplication {
        public:
        protected:
            Server::MyServer* server{nullptr}; //server对象

            Client::MyCommunicator* communicator{nullptr}; //client 对象
        };
    }

}


#endif //MYFRAMEWORK2_MYAPPLICATION_H
