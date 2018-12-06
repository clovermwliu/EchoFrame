//
// Created by mingweiliu on 2018/11/5.
//

#ifndef MYFRAMEWORK2_MYCHANNEL_H
#define MYFRAMEWORK2_MYCHANNEL_H

#include "net/ev/MyLoop.h"
#include "net/MyGlobal.h"
#include "net/socket/MySocket.h"
#include "net/buffer/MySKBuffer.h"
#include "net/ev/MyWatcher.h"
#include "net/buffer/myIOBuf.h"
#include "util/MyTimeProvider.h"

namespace MF {
    namespace Server {
        class EventLoop; //预定义

        class MyChannel : public std::enable_shared_from_this<MyChannel>{
        public:
            typedef std::function<bool (std::shared_ptr<MyChannel>)> OnTimeoutFunc;
            /**
             * 构造函数
             * @param socket socket
             */
            explicit MyChannel(Socket::MySocket *socket);

            virtual ~MyChannel();

            /**
             * 关闭通道
             */
            virtual void close() = 0;

            /**
             * 读取数据
             * @param buf 可读指针地址
             * @param length 可读指针长度
             * @return 0 成功 -1 失败
             */
            virtual int32_t onRead() = 0;

            /**
             * 获取可读数据的长度
             * @return 可读数据长度
             */
            uint32_t getReadableLength() const {
                return readBuf->getReadableLength();
            }

            /**
             * 获取可读数据的指针
             * @param len 需要读取的长度, 并且返回可读的长度
             * @return 指针
             */
            char* getReadableBuffer(uint32_t* len) const {
                return readBuf->readable(len);
            }

            /**
             * 有数据可以发送了
             */
            virtual int32_t onWrite() = 0;

            /**
             * 将可以读的数据读出来
             * @param length
             * @return
             */
            std::unique_ptr<Buffer::MyIOBuf> fetchPacket(uint32_t length);

            /**
             * 发送响应
             * @param buf  buffer
             * @param length  length
             * @return 发送的数据字节数
             */
            virtual uint32_t sendResponse(const char* buf, uint32_t length) = 0;

            /**
             * 设置read watcher
             * @param readWatcher
             */
            void setReadWatcher(EV::MyIOWatcher *readWatcher);

            /**
             * 设置write watcher
             * @param writeWatcher
             */
            void setWriteWatcher(EV::MyAsyncWatcher *writeWatcher);

            /**
             * 设置超时watcher
             * @param timeoutWatcher timeout watcher
             */
            void setTimeoutWatcher(EV::MyTimerWatcher *timeoutWatcher);

            /**
             * 超时定时器重新计时
             * @param timeout timeout
             */
            void resetTimer(uint32_t timeout);

            /**
             * channel超时需要做的事情
             */
            bool checkTimeout();

            /**
             * 设置超时检查函数
             * @param func
             */
            void setOnTimeoutFunc(OnTimeoutFunc&& func);

            /**
             * 获取uid
             * @return uid
             */
            uint64_t getUid() const {
                return uid;
            }

            EventLoop *getLoop() const;

            void setLoop(EventLoop *loop);

            uint32_t getLastReceiveTime() const;

        protected:
            uint64_t uid{0}; //连接的标识id

            Socket::MySocket* socket{nullptr}; //socket
            EV::MyIOWatcher* readWatcher{nullptr}; //connectWatcher;
            EV::MyAsyncWatcher* writeWatcher{nullptr}; //writeWatcher;
            EV::MyTimerWatcher* timeoutWatcher{nullptr}; //timeout watcher
            EventLoop* loop; //事件循环

            //write buffer
            Buffer::MySKBuffer* writeBuf {nullptr};
            std::mutex writeBufMutex;

            //read buffer
            Buffer::MySKBuffer* readBuf {nullptr};

            uint32_t lastReceiveTime{0}; //最近一次接收到消息的时间

            OnTimeoutFunc onTimeoutFunc; //channel超时检查函数
        };

        /**
         * tcp channel
         */
        class MyTcpChannel : public MyChannel {
        public:
            MyTcpChannel(Socket::MySocket *socket);

            int32_t onRead() override;

            int32_t onWrite() override;

            uint32_t sendResponse(const char *buf, uint32_t length) override;

            /**
             * 关闭channel，需要关闭socket和所有watcher
             */
            void close() override;
        };

        /**
         * udp channel
         */
        class MyUdpChannel : public MyChannel {
        public:
            MyUdpChannel(Socket::MySocket* socket, const std::string& ip, uint16_t port);

            int32_t onRead() override;

            int32_t onWrite() override;

            static uint64_t createUid(const std::string& ip, uint16_t port);

            uint32_t sendResponse(const char *buf, uint32_t length) override;

            /**
             * 关闭channel, 不需要关闭socket和readwatcher
             */
            void close() override;

        private:
            std::string ip;
            uint16_t port;
        };
    }
}



#endif //MYFRAMEWORK2_MYCHANNEL_H
