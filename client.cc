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

int main(int argc, const char * argv[]) {

    //初始化comm
    CommConfig commConfig;
    commConfig.handlerThreadCount = 1;
    commConfig.ioThreadCount = 1;
    MyCommunicator::GetInstance()->initialize(commConfig);

    //设置proxy可连接的服务信息
    auto servantName = "testServant";
    ClientConfig clientConfig;
    clientConfig.host = "127.0.0.1";
    clientConfig.port = 8888;
    clientConfig.timeout = 3;
    std::vector<ClientConfig> clientConfigs;
    clientConfigs.push_back(clientConfig);
    MyCommunicator::GetInstance()->update(servantName, clientConfigs);

    //构造一个proxy
    auto proxy = MyCommunicator::GetInstance()->getServantProxy<MyDemoProxy>(servantName);

    //从控制台读取数据
    std::string line;
    for (int i = 0; i < 1000; ++i) {
        std::cin >> line;
        if (!line.empty()) {
            std::string cmd = line.substr(0, line.find(' '));
            std::string msg = line.substr(line.find(' ') + 1);
            auto req = std::move(std::unique_ptr<MyDemoMessage<std::string>>(
                    new MyDemoMessage<std::string>(cmd, msg + "\r\n")));
            auto rsp = std::move(proxy->buildRequest<
                    MyDemoMessage<std::string>, MyDemoMessage<std::string>
                    >(std::move(req))
                    ->executeAndWait());

            LOG(INFO) << "receive response: " << rsp->getCmd() << " " << rsp->getMsg() << std::endl;

        }
    }

    return 0;
}
