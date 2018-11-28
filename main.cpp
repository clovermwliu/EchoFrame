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


using namespace MF::EV;
using namespace MF::Server;
using namespace MF::Buffer;
using namespace MF::Protocol;
using namespace MF::Socket;
using namespace MF::DEMO;

int main(int argc, const char * argv[]) {

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
    servantConfig.timeout = 3;
    servantConfig.handlerThreadCount = 2;
    MyDemoDispatcher* dispatcher = new MyDemoDispatcher();
    if (server->addServant<MyTcpServant>(servantConfig, dispatcher) !=  0) {
        LOG(ERROR) << "create servant fail" << std::endl;
        return 0;
    }

    servantConfig.name = "testServant2";
    servantConfig.host = "0.0.0.0";
    servantConfig.port = 8889;
    servantConfig.timeout = 3;
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
