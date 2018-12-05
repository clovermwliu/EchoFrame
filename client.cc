//
//  main.cpp
//  MF
//
//  Created by mingweiliu on 16/12/26.
//  Copyright (c) 2016年 mingweiliu. All rights reserved.
//

#include <iostream>
#include <thread>
#include <glog/logging.h>
#include <unistd.h>
#include <array>

#include "net/client/MyCommunicator.h"
#include "net/demo/MyDemoProxy.h"
#include "net/demo/MyDemoMessage.h"


using namespace MF::EV;
using namespace MF::Buffer;
using namespace MF::Protocol;
using namespace MF::Socket;
using namespace MF::DEMO;
using namespace MF::Client;

using DemoMsg = std::unique_ptr<MyDemoMessage<std::string>>;
using DemoContext = std::shared_ptr<MySession<MyDemoMessage<std::string>, MyDemoMessage<std::string>>>;

using Action = std::function<int32_t (
        const std::unique_ptr<MyDemoMessage<std::string>>&
                , const std::shared_ptr<MySession<MyDemoMessage<std::string>,MyDemoMessage<std::string>>>&)>;
int main(int argc, const char * argv[]) {

    //初始化comm
    CommConfig commConfig;
    commConfig.handlerThreadCount = 1;
    commConfig.ioThreadCount = 1;
    MyCommunicator::GetInstance()->initialize(commConfig);

    //设置proxy可连接的服务信息
    auto servantName = "testServant";
    ClientConfig clientConfig;
    clientConfig.host = "0.0.0.0";
    clientConfig.port = 8889;
    clientConfig.timeout = 3;
    clientConfig.autoReconnect = true;
    clientConfig.clientType = kClientTypeUdp;
    clientConfig.heartbeatInterval = 5;
    std::vector<ClientConfig> clientConfigs;
//    clientConfigs.push_back(clientConfig);

    //tcp client
    clientConfig.host = "0.0.0.0";
    clientConfig.port = 8888;
    clientConfig.timeout = 3;
    clientConfig.autoReconnect = true;
    clientConfig.clientType = kClientTypeTcp;
    clientConfig.heartbeatInterval = 5;
    clientConfigs.push_back(clientConfig);
    MyCommunicator::GetInstance()->update(servantName, clientConfigs);

    //构造一个proxy
    auto proxy = MyCommunicator::GetInstance()->getServantProxy<MyDemoProxy>(servantName);

    //从控制台读取数据
    std::string line;
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::string line;
//        std::getline(std::cin, line);
//        LOG(INFO) << "read line: " << line << std::endl;
//        if (!line.empty()) {
//            std::string cmd = line.substr(0, line.find(' '));
//            std::string msg = line.substr(line.find(' ') + 1);
//
//            if (cmd == "username") {
//                LOG(INFO) << proxy->setUsername(msg) << std::endl;
//            } else if (cmd == "password") {
//                LOG(INFO) << proxy->setPassword(msg) << std::endl;
//            } else if (cmd == "quit") {
//                proxy->quit();
//            }
//        }
    }

    return 0;
}
