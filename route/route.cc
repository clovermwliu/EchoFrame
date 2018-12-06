//
// Created by mingweiliu on 2018/12/3.
//
#include "route/MyRouteDispatcher.h"
#include "net/server/MyServer.h"
using namespace MF::Route;
using namespace MF::EV;
using namespace MF::Server;
using namespace MF::Buffer;
using namespace MF::Protocol;
using namespace MF::Socket;

int main(int argc, const char * argv[]) {
    MyServer* server = new MyServer();

    //初始化server config
    ServerConfig serverConfig;
    serverConfig.ioThreadCount = 1;
    serverConfig.routeServantName = "udpRouteServant";
    if(server->initServer(serverConfig) != 0) {
        LOG(ERROR) << "init server fail" << std::endl;
        return 0;
    }

    //初始化dispatcher
    ServantConfig servantConfig;
//    servantConfig.name = "tcpRouteServant";
//    servantConfig.host = "0.0.0.0";
//    servantConfig.port = 9998;
//    servantConfig.timeout = 30;
//    servantConfig.handlerThreadCount = 2;
    MyRouteDispatcher* dispatcher = new MyRouteDispatcher();
//    if (server->addServant<MyTcpServant>(servantConfig, dispatcher) !=  0) {
//        LOG(ERROR) << "create servant fail" << std::endl;
//        return 0;
//    }

    servantConfig.name = "udpRouteServant";
    servantConfig.host = "0.0.0.0";
    servantConfig.port = 9999;
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
