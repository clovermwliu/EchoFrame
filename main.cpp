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

#include "net/server/MyServer.h"
#include "net/demo/MyDemoDispatcher.h"
#include "net/client/MyCommunicator.h"


using namespace MF::EV;
using namespace MF::Server;
using namespace MF::Buffer;
using namespace MF::Protocol;
using namespace MF::Socket;
using namespace MF::Client;
using namespace MF::DEMO;

int main(int argc, const char * argv[]) {
    //初始化client
    CommConfig commConfig;
    commConfig.handlerThreadCount = 2;
    commConfig.ioThreadCount = 1;
    MyCommunicator::GetInstance()->initialize(commConfig);

    //设置proxy可连接的服务信息
    auto servantName = "udpRouteServant";
    ClientConfig clientConfig;
    clientConfig.host = "0.0.0.0";
    clientConfig.port = 9999;
    clientConfig.timeout = 3;
    clientConfig.autoReconnect = true;
    clientConfig.clientType = kClientTypeUdp;
    clientConfig.needHeartbeat = false;
    std::vector<ClientConfig> clientConfigs;
    clientConfigs.push_back(clientConfig);
    MyCommunicator::GetInstance()->update(servantName, clientConfigs);

    MyServer* server = new MyServer();

    //初始化server config
    ServerConfig serverConfig;
    serverConfig.ioThreadCount = 1;
    if(server->initServer(serverConfig) != 0) {
        LOG(ERROR) << "init server fail" << std::endl;
        return 0;
    }

    //初始化dispatcher
    ServantConfig servantConfig;
    servantConfig.name = "testServant";
    servantConfig.host = "0.0.0.0";
    servantConfig.port = 8888;
    servantConfig.timeout = 10;
    servantConfig.handlerThreadCount = 2;
    MyDemoDispatcher* dispatcher = new MyDemoDispatcher();
    if (server->addServant<MyTcpServant>(servantConfig, dispatcher) !=  0) {
        LOG(ERROR) << "create servant fail" << std::endl;
        return 0;
    }

    servantConfig.name = "testServant2";
    servantConfig.host = "0.0.0.0";
    servantConfig.port = 8889;
    servantConfig.timeout = 30;
    servantConfig.handlerThreadCount = 2;
    if (server->addServant<MyUdpServant>(servantConfig, dispatcher) !=  0) {
        LOG(ERROR) << "create servant fail" << std::endl;
        return 0;
    }

    //启动server
    if (server->startServer() != 0) {
        LOG(ERROR) << "start server fail" << std::endl;
        return 0;
    }

    //等待server退出
    server->wait();
}
