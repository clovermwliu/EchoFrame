//
// Created by mingweiliu on 2018/11/14.
//

#ifndef MYFRAMEWORK2_MYPROXY_H
#define MYFRAMEWORK2_MYPROXY_H

#include <deque>
#include "net/client/MyClient.h"
#include "net/client/MyRequest.h"
#include "util/MyThreadPool.h"
#include "net/protocol/MyMessage.h"
#include "util/MyQueue.h"

namespace MF {
    namespace Client{

        class ClientLoopManager; //client loop 循环

        class ClientLoop; //事件循环

        struct ProxyConfig {
            std::string servantName; //servant name
            uint32_t asyncTimeout{3};  //asyncTimeout, 默认3秒超时
            uint32_t reconnectInterval {10}; //重连间隔
            std::vector<ClientConfig> clients; //client 配置
            uint32_t handlerThreadCount{1}; //handler thread count
        };

        /**
         * 一个proxy管理多个client，指向某个特定服务的集群
         */
        class MyProxy : public std::enable_shared_from_this<MyProxy>{
        public:
            MyProxy(ClientLoopManager* loop);

            virtual ~MyProxy();

            /**
             * 获取servant name
             * @return servant name
             */
            const std::string& getServantName() const {
                return config.servantName;
            }

            /**
             * 设置handler executor
             * @param executor executor
             */
            void setHandlerExecutor(MyThreadExecutor<int32_t>* executor) {
                this->handlerExecutor = executor;
            }

            /**
             * 分发数据包
             * @param iobuf iobuf
             */
            virtual void handlePacket(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) = 0;

            /**
             * 更新Config配置
             * @param config config
             */
            virtual void update(const ProxyConfig& config);

        protected:

            /**
             * 增加tcp client
             * @param config config
             * @return
             */
            virtual int32_t addTcpClient(const ClientConfig& config);

            /**
             * 增加udp client
             * @param config config
             * @return 0 成功 其他失败
             */
            virtual int32_t addUdpClient(const ClientConfig& config);

            /**
             * 获取随机的client
             * @return client
             */
            std::shared_ptr<MyClient> getClient();

            /**
             * 有数据可读
             */
            virtual void onRead(EV::MyWatcher* watcher);

            /**
             * 客户端断连
             */
            virtual void onDisconnect(const std::shared_ptr<MyClient>& client);

            /**
             * 判断响应包数据是否完整
             * @param buf
             * @param len
             * @return
             */
            virtual int32_t isPacketComplete(const char* buf, uint32_t len) = 0;

            /**
             * 获取packet length
             * @param buf buf
             * @param len length
             * @return 数据包的长度
             */
            virtual uint32_t getPacketLength(const char* buf, uint32_t len) = 0;

        protected:
            MyThreadQueue<std::weak_ptr<MyClient>> clients; //可以使用的client

            ClientLoopManager* loops; //事件循环

            MyThreadExecutor<int32_t >* handlerExecutor; //handler的执行线程池

            ProxyConfig config; //proxy的配置

            std::hash<std::string> hash;
        };

        /**
         * 每个servant对应的proxy
         */
        class ServantProxy: public MyProxy {
        public:
            /**
             * 构造函数
             * @param servantName  servantName
             * @param loop 事件循环
             */
            ServantProxy(ClientLoopManager* loop);

            /**
             * 初始化proxy
             * @return 初始化结果
             */
            int32_t initialize(const std::vector<ClientConfig>& config);

            /**
             * 处理packet
             * @param iobuf iobuf
             * @return 处理结果
             */
            void handlePacket(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) override;

            /**
             * 更新proxy
             * @param config config
             */
            void update(const ProxyConfig &config) override;

        protected:
            /**
             * 发送数据
             */
            template<typename REQ, typename RSP>
            std::shared_ptr<MyRequest<REQ, RSP>> buildRequest(std::unique_ptr<REQ> message);

            /**
             * decode 消息
             * @param iobuf iobuf
             * @return 消息指针
             */
            virtual std::unique_ptr<Protocol::MyMessage> decode(const std::unique_ptr<Buffer::MyIOBuf>& iobuf) = 0;

            /**
             * 编码消息
             * @param message 消息
             * @return buffer
             */
            virtual std::unique_ptr<Buffer::MyIOBuf> encode(const std::unique_ptr<Protocol::MyMessage>& message) = 0;

        private:
            void handleMessage(const char* buf, uint32_t len, std::shared_ptr<MyClient> client);
        };

        template<typename REQ, typename RSP>
        std::shared_ptr<MyRequest<REQ, RSP>> ServantProxy::buildRequest(std::unique_ptr<REQ> message) {
            //1. 获取client
            auto client = getClient();
            auto request = std::shared_ptr<MyRequest<REQ, RSP>>(
                    new MyRequest<REQ, RSP>(std::weak_ptr<MyClient>(client)));
            if (client == nullptr) {
                LOG(ERROR) << "all clients are disconnected" << std::endl;
                return request;
            }

            //2. encode
            auto m = std::move(std::unique_ptr<Protocol::MyMessage>(
                    dynamic_cast<Protocol::MyMessage*>(message.release())));
            auto payload = encode(m);
            if (payload == nullptr) {
                LOG(ERROR) << "encode message fail, uid: " << request->getRequestId() << std::endl;
                return nullptr;
            }

            //3. 设置request并且设置等待超时时长
            client->addRequest(request);
            auto weakRequest = std::weak_ptr<MyRequest<REQ, RSP>> (request);
            auto watcher = EV::MyWatcherManager::GetInstance()->create<EV::MyTimerWatcher>(
                    [weakRequest](EV::MyWatcher* watcher) -> void {
                        //1. 检查request是否有效
                        auto r = weakRequest.lock();
                        if (r == nullptr) {
                            //request已经被删除了
                            return;
                        }

                        LOG(ERROR) << "request timeout, requestId: " << r->getRequestId() << std::endl;
                        //2. 执行超时任务
                        r->doTimeoutAction();

                    }, config.asyncTimeout, 0);

            //保存timeout watcher
            request->setTimeoutWatcher(watcher);

            //4. 增加数据包头
            auto magicMsg = std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage());
            magicMsg->setLength(magicMsg->headLen() + payload->getReadableLength());
            magicMsg->setVersion(static_cast<uint16_t >(1)); //TODO: 版本控制
            magicMsg->setIsRequest(static_cast<int8_t >(1));
            magicMsg->setRequestId(request->getRequestId());
            magicMsg->setServerNumber(0); //TODO: 先设置为0

            //5. 编码数据包
            magicMsg->setPayload(std::move(payload));

            //6. 保存数据包
            request->setRequest(std::move(magicMsg));

            return request; //返回request
        }
    }
}


#endif //MYFRAMEWORK2_MYPROXY_H
