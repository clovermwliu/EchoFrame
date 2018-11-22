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

namespace MF {

    namespace Client {

        class ClientLoop; //client loop

        class MyBaseRequest; //request

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

            int32_t clientType{kClientTypeTcp};
        };
        class MyClient : public std::enable_shared_from_this<MyClient>{
        public:
            /**
             * 构造函数
             */
            MyClient(uint16_t servantId) {
                socket = new Socket::MySocket();
                uid = static_cast<uint32_t >(socket->getfd() << 16 | servantId);
                readBuffer = new Buffer::MySKBuffer(g_default_skbuffer_capacity);
            }

            /**
             * 析构函数
             */
            virtual ~MyClient() {
                if (socket != nullptr) {
                    delete(socket);
                }

                if (connectWatcher != nullptr) {
                    EV::MyWatcherManager::GetInstance()->destroy(connectWatcher); //销毁read watcher
                }

                if (readBuffer != nullptr) {
                    delete(readBuffer);
                }

                if (readWatcher != nullptr) {
                    delete(readWatcher);
                }
            }

            uint32_t getUid() const {
                return uid;
            }

            const ClientConfig& getConfig() const {
                return config;
            }

            uint32_t getServantId() const {
                return uid & 0xFFFF;
            }

            Socket::MySocket *getSocket() const {
                return socket;
            }

            EV::MyIOWatcher *getReadWatcher() const {
                return readWatcher;
            }

            void setReadWatcher(EV::MyIOWatcher *readWatcher) {
                MyClient::readWatcher = readWatcher;
            }

            ClientLoop *getLoop() const {
                return loop;
            }

            uint32_t getConnectTime() const;

            /**
             * 新增request
             * @param request request
             */
            void addRequest(const std::shared_ptr<MyBaseRequest>& request );

            /**
             * 删除一个request
             * @param request request
             */
            void removeRequest(uint64_t requestId) ;

            /**
             * 查询request
             * @param requestId requestId
             * @return request
             */
            std::shared_ptr<MyBaseRequest> findRequest(uint64_t requestId) ;

            /**
             * 初始化client
             * @param config config
             * @return 结果
             */
            virtual int32_t connect(const ClientConfig& config, ClientLoop* loop) = 0;

            /**
             * 异步connect
             * @param config config
             * @return 返回数据
             */
            virtual std::future<int32_t > asyncConnect(const ClientConfig& config, ClientLoop* loop) = 0;

            /**
             * 有数据可以读
             * @return 读取结果
             */
            virtual int32_t onRead(char** buf, uint32_t* len) = 0;

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
            virtual int32_t sendPayload(const std::unique_ptr<Buffer::MyIOBuf>& iobuf) {
                uint32_t length = iobuf->getReadableLength();
                char* buf = static_cast<char*>(iobuf->readable());
                return sendPayload(buf, length);
            }

            /**
             * 取出数据包
             * @param length length
             * @return 0 成功 other 失败
             */
            virtual std::unique_ptr<Buffer::MyIOBuf> fetchPayload( uint32_t length) = 0;
        protected:

            /**
             * 有数据可以读
             * @param watcher watcher
             */
            virtual void onConnect(EV::MyWatcher *watcher) = 0;

        protected:
            uint32_t uid{0}; //uid

            Socket::MySocket* socket{nullptr}; //socket
            std::promise<int32_t > connectPromise; //connect promise
            EV::MyIOWatcher* connectWatcher{nullptr}; //connect watcher
            EV::MyIOWatcher* readWatcher{nullptr}; //read watcher

            Buffer::MySKBuffer* readBuffer{nullptr}; //read buffer

            ClientLoop* loop{nullptr}; //ev loop

            //配置
            ClientConfig config;

            std::map<uint64_t ,std::shared_ptr<MyBaseRequest>> requests;// 已经发出的所有request

            uint32_t connectTime; //连接开始的时间
        };

        /**
         * tcp client
         */
        class MyTcpClient : public MyClient {
        public:
            MyTcpClient(uint16_t servantId);

            int32_t connect(const ClientConfig &config, ClientLoop* loop) override;

            std::future<int32_t> asyncConnect(const ClientConfig &config, ClientLoop* loop) override;

            int32_t onRead(char** buf, uint32_t* len) override;

            int32_t sendPayload(const char *buffer, uint32_t length) override;

            std::unique_ptr<Buffer::MyIOBuf> fetchPayload(uint32_t length) override;

        protected:
            void onConnect(EV::MyWatcher *watcher) override;
        };
    }
}


#endif //MYFRAMEWORK2_MYCLIENT_H
