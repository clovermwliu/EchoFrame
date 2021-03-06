//
// Created by mingweiliu on 2018/11/19.
//

#ifndef MYFRAMEWORK2_MYCOMMUNICATOR_H
#define MYFRAMEWORK2_MYCOMMUNICATOR_H

#include "util/MySingleton.h"
#include "net/client/MyProxy.h"
#include "net/client/ClientLoop.h"

namespace MF {
    namespace Client {

        struct CommConfig {
            uint32_t handlerThreadCount{1}; //handler的线程数
            uint32_t ioThreadCount{1}; //io线程池
        };

        /**
         * 通信器，用于管理多个Proxy
         */
        class MyCommunicator {
        public:

            void initialize(const CommConfig& config){
                this->config = config;
                handlerExecutor = new MyThreadExecutor<int32_t >(config.handlerThreadCount);
                loops = new ClientLoopManager();
                if (loops->initialize(config.ioThreadCount)) {
                    LOG(ERROR) << "initialize io thread fail" << std::endl;
                }
            }

            /**
             * 更新proxy配置
             * @param servantName servant name
             * @param config config
             */
            void update(const std::string& servantName, const std::vector<ClientConfig>& config) {
                std::lock_guard<std::mutex> guard(mutex);
                //1. 保存起来
                ProxyConfig proxyConfig;
                proxyConfig.servantName = servantName;
                proxyConfig.clients = config;
                proxyConfigs[servantName] = proxyConfig;

                //2. 新proxy只做保存
                if (proxys.find(servantName) == proxys.end()) {
                    return ;
                }

                //3. 更新旧的proxy
                auto proxy = proxys[servantName];

                //更新配置
                proxy->update(proxyConfig); //更新配置
            }

            /**
             * 获取一个ServantProxy
             * @tparam T proxy的类型
             * @param servantName servant name
             * @return
             */
            template<typename T>
            std::shared_ptr<T> getServantProxy(const std::string& servantName);

        protected:
            std::map<std::string, std::shared_ptr<ServantProxy>> proxys; //已经建立的proxys

            std::mutex mutex; //proxy 锁

            MyThreadExecutor<int32_t >*  handlerExecutor; //executor

            ClientLoopManager* loops; //客户端线程池

            CommConfig config; //配置

            std::map<std::string, ProxyConfig> proxyConfigs;
        };

        template<typename T>
        std::shared_ptr<T> MyCommunicator::getServantProxy(const std::string &servantName) {
            std::lock_guard<std::mutex> guard(mutex);
            //1. 检查map中是否已经存在
            if (proxys.find(servantName) != proxys.end()) {
                return dynamic_pointer_cast<T>(proxys[servantName]);
            }

            //2. 检查是否有配置
            if (proxyConfigs.find(servantName) == proxyConfigs.end()) {
                return nullptr;
            }

            //2. 构造新的proxy
            auto proxy = dynamic_pointer_cast<ServantProxy>(std::make_shared<T>(loops));
            //设置handler
            proxy->setHandlerExecutor(this->handlerExecutor);

            //3. 保存到map
            proxys[servantName] = proxy;

            //4. 初始化proxy
            proxy->update(proxyConfigs[servantName]);
            return dynamic_pointer_cast<T>(proxy);
        }
    }
}


#endif //MYFRAMEWORK2_MYCOMMUNICATOR_H
