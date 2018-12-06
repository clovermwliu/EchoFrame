//
//  myserver.h
//  MyFramewrok
//  管理所有的servant和Handler
//  Created by mingweiliu on 2017/2/17.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myserver_h
#define myserver_h

#include "util/MyCommon.h"
#include "util/MyThreadPool.h"
#include "net/server/MyServant.h"
#include "net/server/EventLoop.h"

namespace MF {
    namespace Server {

        /**
         * server启动配置
         */
        struct ServerConfig {
            uint32_t ioThreadCount{0}; //io线程数
            std::string routeServantName; //route servant name
        };

        /**
         * server端
         */
        class MyServer {
        public:
            /**
             * 初始化Server
             * @param config config
             * @return 0成功 其他失败
             */
            int32_t initServer(const ServerConfig& config);

            /**
             * 构造servant
             * @param config servant 配置
             * @return 0 成功 其他 失败
             */
            template<typename Servant>
            int32_t addServant(const ServantConfig &config, MyDispatcher *dispatcher);

            /**
             * 启动服务
             * @return 0 成功 其他 失败
             */
            int32_t startServer();

            /**
             * 等待server退出
             */
            void wait();
        protected:
            std::map<std::string, MyServant*> servantMap;//服务列表
            EventLoopManager* loopManager {nullptr}; //事件循环列表
            ServerConfig config; //server配置
        };

        template <typename Servant>
        int32_t MyServer::addServant(const MF::Server::ServantConfig &config, MyDispatcher *dispatcher) {
            //1. new 一个对象
            MyServant* servant = new Servant(this->loopManager, dispatcher);

            //2. 初始化servant
            int32_t rv = servant->initialize(config);
            if (rv != 0) {
                LOG(ERROR) << "initialize servant fail, name: " << config.name
                           << ", host: "  << config.host << ", port: " << config.port << std::endl;
                return rv;
            }

            //3. 将servant 保存起来
            servantMap[config.name] = servant;
            return rv;
        }
    }
}

#endif /* myserver_h */
