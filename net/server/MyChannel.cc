//
// Created by mingweiliu on 2018/11/5.
//

#include "net/server/MyChannel.h"
#include "util/MyCommon.h"
#include "util/MyTimeProvider.h"
#include "net/server/EventLoop.h"

namespace MF {
    namespace Server {

        MyChannel::MyChannel(Socket::MySocket *socket) : socket(socket) {
            this->uid = static_cast<uint32_t >(socket->getfd()); //TODO: 先设置成fd
            this->readBuf = new Buffer::MySKBuffer(g_default_skbuffer_capacity);
            this->writeBuf= new Buffer::MySKBuffer(g_default_skbuffer_capacity);

            this->lastReceiveTime = MyTimeProvider::now();
        }

        MyChannel::~MyChannel() {
            if (this->readBuf != nullptr) {
                delete(this->readBuf);
            }

            if (this->writeBuf != nullptr) {
                delete(this->writeBuf);
            }
        }

        void MyChannel::close() {
            if (this->socket != nullptr) {
                delete(this->socket);
            }

            if (readWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(readWatcher);
            }

            if (writeWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(writeWatcher);
            }

            if (timeoutWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(timeoutWatcher);
            }
        }

        uint32_t MyChannel::fetchPacket(char **buf, uint32_t length) {
            uint32_t readLen = length;
            *buf = readBuf->getReadableAndMove(&readLen);

            return readLen;
        }

        std::unique_ptr<Buffer::MyIOBuf> MyChannel::fetchPacket(uint32_t length) {
            uint32_t readLen = length;
            char* tmp = readBuf->getReadableAndMove(&readLen);

            std::unique_ptr<Buffer::MyIOBuf> buf = Buffer::MyIOBuf::create(readLen);
            buf->write<char*>(tmp, readLen);
            return std::move(buf);
        }

        uint32_t MyChannel::sendResponse(const char *buf, uint32_t length) {
            std::lock_guard<std::mutex> guard(writeBufMutex); //尝试加锁
            char* tmp = this->writeBuf->getWriteableAndMove(length);
            if (tmp != nullptr) {
                memcpy(tmp, buf, length); //拷贝数据

                //发送可写请求
                this->writeWatcher->signal();
            }

            return length;
        }

        /**
         * 设置读数据watcher
         * @param readWatcher
         */
        void MyChannel::setReadWatcher(EV::MyIOWatcher *readWatcher) {
            MyChannel::readWatcher = readWatcher;
        }

        /**
         * 设置写数据watcher
         * @param writeWatcher
         */
        void MyChannel::setWriteWatcher(EV::MyAsyncWatcher *writeWatcher) {
            MyChannel::writeWatcher = writeWatcher;
        }

        EV::MyTimerWatcher *MyChannel::getTimeoutWatcher() const {
            return timeoutWatcher;
        }

        void MyChannel::setTimeoutWatcher(EV::MyTimerWatcher *timeoutWatcher) {
            MyChannel::timeoutWatcher = timeoutWatcher;
        }

        EventLoop* MyChannel::getLoop() const {
            return loop;
        }

        void MyChannel::setLoop(EventLoop *loop) {
            MyChannel::loop = loop;
        }

        uint64_t MyChannel::getLastReceiveTime() const {
            return lastReceiveTime;
        }

        MyTcpChannel::MyTcpChannel(Socket::MySocket *socket) : MyChannel(socket) {}

        int32_t MyTcpChannel::onRead(char **buf, uint32_t *length) {
            int32_t  result = -1;
            //1. 从socket读取数据
            uint32_t readLen = 1024; //每次读取1k
            char* tmp = readBuf->writeable(readLen);
            int32_t rv = socket->read(tmp, readLen);
            if (rv == 0) {
                //对端已关闭
            } else if (rv  < 0) {
                //读取出错
                result = errno == EAGAIN ? 0 : -1;
            } else {
                //读到了数据
                result = 0;
                readBuf->moveWriteable(static_cast<uint32_t >(rv));
            }

            //获取可读数据
            *length = readBuf->getReadableLength();
            *buf = readBuf->readable(length);

            lastReceiveTime = MyTimeProvider::now(); //设置最后一次接收到消息的时间
            return result;
        }

        int32_t MyTcpChannel::onWrite() {
            std::lock_guard<std::mutex> guard(writeBufMutex); //尝试加锁
            //1. 获取可发送的长度
            uint32_t len = this->writeBuf->getReadableLength();
            char* buf = this->writeBuf->readable(&len);
            uint32_t send = 0;
            while(send < len) {
                int32_t rv = socket->write(static_cast<void*>(buf + send), len - send);
                if (rv <= 0) {
                    break;
                }

                send += rv; //保存已发送的数据长度
            }

            //2. 移动指针
            writeBuf->moveReadable(len);
            return send >= len ? 0 : -1; //检查发送的字节数和可发送的字节数是否一致
        }
    }
}
