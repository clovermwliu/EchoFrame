//
// Created by mingweiliu on 2018/11/14.
//

#ifndef MYFRAMEWORK2_MYPROXY_H
#define MYFRAMEWORK2_MYPROXY_H

#include <unordered_map>
#include "net/client/MyClient.h"
#include "net/client/MySession.h"
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

        enum ProxyStatus : uint32_t  {
            kProxyStatusDisconnected = 0,
            kProxyStatusConnected = 1,
        };

        typedef std::function<void (std::weak_ptr<MyClient>)> ProxyFunc; //proxy可执行代码

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
             * 增加client
             * @param client client
             * @return 0 成功 其他失败
             */
            int32_t addClient(const MF::Client::ClientConfig &config, std::shared_ptr<MyClient> client);

            /**
             * 获取随机的client
             * @return client
             */
            std::shared_ptr<MyClient> getClient();

            /**
             * 获取可用的client数量
             * @return
             */
            uint32_t getConnectedClientCount() const;

            /**
             * 有数据可读
             */
            virtual void onRead(EV::MyWatcher* watcher);

            /**
             * 客户端连接成功回调
             * @param client client
             */
            virtual void onConnect(std::shared_ptr<MyClient> client) {};

            /**
             * 客户端断连
             */
            virtual void onDisconnect(const std::shared_ptr<MyClient>& client);

            /**
             * 开启心跳
             */
            virtual void heartbeat(std::shared_ptr<MyClient> client) = 0;

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
            MyClientSelector selector; //client选择器

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
             * 客户端连接成功回调
             * @param client client
             */
            virtual void onConnect(std::shared_ptr<MyClient> client);

            /**
             * 发送数据
             */
            template<typename REQ, typename RSP>
            std::shared_ptr<MySession<REQ, RSP>> buildSession(std::unique_ptr<REQ> message);

            /**
             * 构造心跳session
             * @return 心跳session
             */
            std::shared_ptr<MySession<void, void>> buildHeartbeatSession(std::shared_ptr<MyClient> client);

            /**
             * 开启心跳
             * @param client client
             */
            void heartbeat(std::shared_ptr<MyClient> client) override;

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
        };

        template<typename REQ, typename RSP>
        std::shared_ptr<MySession<REQ, RSP>> ServantProxy::buildSession(std::unique_ptr<REQ> message) {
            //1. 获取client
            auto client = getClient();
            auto session = std::shared_ptr<MySession<REQ, RSP>>(
                    new MySession<REQ, RSP>(std::weak_ptr<MyClient>(client)));
            if (client == nullptr) {
                LOG(ERROR) << "all clients are disconnected" << std::endl;
                return session;
            }

            //2. encode
            auto m = std::move(std::unique_ptr<Protocol::MyMessage>(
                    dynamic_cast<Protocol::MyMessage*>(message.release())));
            auto payload = encode(m);
            if (payload == nullptr) {
                LOG(ERROR) << "encode message fail, uid: " << session->getRequestId() << std::endl;
                return nullptr;
            }

            //3. 设置request并且设置等待超时时长
            client->addSession(session);
            auto weakRequest = std::weak_ptr<MySession<REQ, RSP>> (session);
            auto watcher = EV::MyWatcherManager::GetInstance()->create<EV::MyTimerWatcher>(
                    [weakRequest](EV::MyWatcher* watcher) -> void {
                        //1. 检查request是否有效
                        auto r = weakRequest.lock();
                        if (r == nullptr) {
                            //request已经被删除了
                            return;
                        }

                        LOG(ERROR) << "session timeout, requestId: " << r->getRequestId() << std::endl;
                        //2. 执行超时任务
                        r->doTimeoutAction();

                    }, config.asyncTimeout, 0);

            //保存timeout watcher
            session->setTimeoutWatcher(watcher);

            //4. 增加数据包头
            auto magicMsg = std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage());
            magicMsg->setLength(magicMsg->headLen() + payload->getReadableLength());
            magicMsg->setFlag(Protocol::kFlagData);
            magicMsg->setVersion(static_cast<uint16_t >(1)); //TODO: 版本控制
            magicMsg->setIsRequest(static_cast<int8_t >(1));
            magicMsg->setRequestId(session->getRequestId());
            magicMsg->setServerNumber(0); //TODO: 先设置为0

            //5. 编码数据包
            magicMsg->setPayload(std::move(payload));

            //6. 保存数据包
            session->setRequest(std::move(magicMsg));

            return session; //返回request
        }
    }
}


#endif //MYFRAMEWORK2_MYPROXY_H
