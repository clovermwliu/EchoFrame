//
// Created by mingweiliu on 2018/11/14.
//

#ifndef MYFRAMEWORK2_MYCLIENT_H
#define MYFRAMEWORK2_MYCLIENT_H

#include <future>
#include "net/MyGlobal.h"
#include "net/socket/MySocket.h"
#include "net/buffer/myIOBuf.h"
#include "net/ev/MyWatcher.h"
#include "util/MyTimeProvider.h"

namespace MF {

    namespace Client {

        class MyClient;

        using OnConnectFunction = std::function<void (std::shared_ptr<MyClient>)>; //连接成功需要进行的操作

        using ReadFunction = std::function<void (EV::MyWatcher*)>; //有数据可读

        class ClientLoop; //client loop

        class MyBaseSession; //request

        typedef enum enumClientType: int32_t {
            kClientTypeTcp = 0, //tcp
            kClientTypeUdp = 1, //udp
        }ClientType;
        /**
         * client配置
         */
        struct ClientConfig{
            std::string host;
            uint16_t port{0};
            uint32_t timeout{3};
            int32_t clientType{kClientTypeTcp}; //连接类型
            bool autoReconnect{true}; //自动重连
            bool needHeartbeat{true}; //是否需要心跳
            uint32_t heartbeatInterval {30}; //心跳间隔
        };
        class MyClient : public std::enable_shared_from_this<MyClient>{
        public:

            typedef std::function<void (std::shared_ptr<MyClient>)> OnHeartbeatFunc; //心跳处理函数
            /**
             * 构造函数
             */
            MyClient(uint16_t servantId) {
                socket = new Socket::MySocket();
                readBuffer = new Buffer::MySKBuffer(g_default_skbuffer_capacity);
            }

            /**
             * 析构函数
             */
            virtual ~MyClient() {
                if (connectWatcher != nullptr) {
                    EV::MyWatcherManager::GetInstance()->destroy(connectWatcher); //销毁read watcher
                }

                if (readBuffer != nullptr) {
                    delete(readBuffer);
                }

                //重置数据
                lastHeartbeatTime = 0;
                loadAvg = 0;
            }

            uint64_t getUid() const {
                return uid;
            }

            const ClientConfig& getConfig() const {
                return config;
            }

            uint32_t getFd() const {
                return static_cast<uint32_t >(socket->getfd());
            }

            uint32_t getServantId() const {
                return static_cast<uint32_t >(uid & 0xFFFF);
            }

            Socket::MySocket *getSocket() const {
                return socket;
            }

            void setReadWatcher(EV::MyIOWatcher *readWatcher) {
                MyClient::readWatcher = readWatcher;
            }

            ClientLoop *getLoop() const {
                return loop;
            }

            uint32_t getLoadAvg() const;

            void setLoadAvg(uint32_t loadAvg);

            uint32_t getLastHeartbeatTime() const;

            void setHeartbeatFunc(OnHeartbeatFunc&& heartbeatFunc);

            void setLastHeartbeatTime(uint32_t lastHeartbeatTime);

            bool needSendHeartbeat() const {
                return config.needHeartbeat
                    && lastHeartbeatTime + config.heartbeatInterval <= MyTimeProvider::now();
            }

            bool connected() const {
                return socket != nullptr && socket->connected();
            }

            /**
             * 新增request
             * @param request request
             */
            void addSession(const std::shared_ptr<MyBaseSession> &request);

            /**
             * 删除一个request
             * @param request request
             */
            void removeSession(uint64_t requestId) ;

            /**
             * 查询request
             * @param requestId requestId
             * @return request
             */
            std::shared_ptr<MyBaseSession> findSession(uint64_t requestId) ;

            /**
             * 初始化client
             * @param config config
             * @return 结果
             */
            virtual int32_t connect(const ClientConfig& config, ClientLoop* loop) = 0;

            /**
             * 断开连接
             */
            virtual void disconnect() = 0;

            /**
             * 重新链接
             */
            virtual void reconnect() = 0;

            /**
             * 异步connect
             * @param config config
             * @return 返回数据
             */
            virtual std::future<int32_t > asyncConnect(
                    const ClientConfig& config, ClientLoop* loop, OnConnectFunction && func) = 0;

            /**
             * 有数据可以读
             * @return 读取结果
             */
            virtual int32_t onRead() = 0;

            /**
             * 获取可读数据的长度
             * @return 可读数据长度
             */
            uint32_t getReadableLength() const {
                return readBuffer->getReadableLength();
            }

            /**
             * 获取可读数据的指针
             * @param len 需要读取的长度, 并且返回可读的长度
             * @return 指针
             */
            char* getReadableBuffer(uint32_t* len) const {
                return readBuffer->readable(len);
            }

            /**
             * 发送数据包
             * @param buffer buffer
             * @param length length
             * @return 0成功 其他失败
             */
            virtual int32_t sendPayload(const char* buffer, uint32_t length) = 0;

            /**
             * 发送数据包
             * @param iobuf iobuf
             * @return 0 成功 其他 失败
             */
            virtual int32_t sendPayload(std::unique_ptr<Buffer::MyIOBuf> iobuf) = 0;

            /**
             * 获取一个完整的数据包
             * @param length length
             * @return 数据包
             */
            virtual std::unique_ptr<Buffer::MyIOBuf> fetchPayload(uint32_t length);

            /**
             * 当超时时需要做的事情
             * @param pred pred
             */
            void whenSessionTimeout(std::function<void()> &&pred, uint64_t requestId);

            /**
             * 处理心跳
             */
            virtual void onHeartbeat();
        protected:

            /**
             * 有数据可以读
             * @param watcher watcher
             */
            virtual void onConnect(EV::MyWatcher *watcher) {};

        protected:
            uint64_t uid{0}; //uid

            Socket::MySocket* socket{nullptr}; //socket
            std::promise<int32_t >* connectPromise; //connect promise
            EV::MyIOWatcher* connectWatcher{nullptr}; //connect watcher
            EV::MyIOWatcher* readWatcher{nullptr}; //read watcher

            Buffer::MySKBuffer* readBuffer{nullptr}; //read buffer

            ClientLoop* loop{nullptr}; //ev loop

            //配置
            ClientConfig config;

            std::map<uint64_t ,std::shared_ptr<MyBaseSession>> requests;// 已经发出的所有request

            uint32_t connectTime; //连接开始的时间

            OnConnectFunction onConnectFunc; //连接成功需要执行的方法

            uint32_t loadAvg{0}; //连接的平均负载 0-100

            uint32_t lastHeartbeatTime{0}; //上次心跳时间

            OnHeartbeatFunc heartbeatFunc; //心跳处理函数
        };

        /**
         * tcp client
         */
        class MyTcpClient : public MyClient {
        public:
            MyTcpClient(uint16_t servantId);

            virtual ~MyTcpClient();

            int32_t connect(const ClientConfig &config, ClientLoop* loop) override;

            void disconnect() override;

            void reconnect() override;

            std::future<int32_t> asyncConnect(
                    const ClientConfig &config, ClientLoop* loop, OnConnectFunction && csf) override;

            int32_t onRead() override;

            int32_t sendPayload(const char *buffer, uint32_t length) override;

            int32_t sendPayload(std::unique_ptr<Buffer::MyIOBuf> iobuf) override;

        protected:
            void onConnect(EV::MyWatcher *watcher) override;
        };

        class MyUdpClient : public MyClient {
        public:
            MyUdpClient(uint16_t servantId);

            ~MyUdpClient() override;

            int32_t connect(const ClientConfig &config, ClientLoop* loop) override;

            std::future<int32_t>
            asyncConnect(const ClientConfig &config, ClientLoop *loop, OnConnectFunction &&func) override;

            void disconnect() override;

            void reconnect() override;

            int32_t onRead() override;

            int32_t sendPayload(const char *buffer, uint32_t length) override;

            int32_t sendPayload(std::unique_ptr<Buffer::MyIOBuf> iobuf) override;

        protected:
        };

        /**
         * client选择器
         */
        class MyClientSelector {
        public:
            /**
             * 增加一个client
             * @param client cient
             */
            void addClient(std::shared_ptr<MyClient> client);

            /**
             * 根据uid删除client
             * @param uid uid
             */
            void removeClient(uint64_t uid);

            /**
             * 根据策略选择一个client
             * @param strategy 策略
             * @return client
             */
            std::shared_ptr<MyClient> getClient(uint32_t strategy);
        private:
            std::map<uint64_t , std::weak_ptr<MyClient>> clients; //所有的客户端
            std::mutex mutex; //锁
        };
    }
}


#endif //MYFRAMEWORK2_MYCLIENT_H
