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

        std::unique_ptr<Buffer::MyIOBuf> MyChannel::fetchPacket(uint32_t length) {
            uint32_t readLen = length;
            char* tmp = readBuf->getReadableAndMove(&readLen);

            std::unique_ptr<Buffer::MyIOBuf> buf = Buffer::MyIOBuf::create(readLen);
            buf->write<char*>(tmp, readLen);
            return std::move(buf);
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

        void MyChannel::setTimeoutWatcher(EV::MyTimerWatcher *timeoutWatcher) {
            MyChannel::timeoutWatcher = timeoutWatcher;
        }

        void MyChannel::resetTimer(uint32_t timeout) {
            if (MyTimeProvider::now() - lastReceiveTime >= 1) {
                //1 秒才修改一次定时器
                LOG(INFO) << "reset timer, timeout: " << timeout << std::endl;
                timeoutWatcher->stop();
                timeoutWatcher->set(timeout, 0);
                timeoutWatcher->start();
            } else {
                LOG(INFO) << "now: " << MyTimeProvider::now() << ", lastReceiveTime: " << lastReceiveTime << std::endl;
            }
        }

        bool MyChannel::checkTimeout() {
            return onTimeoutFunc ? onTimeoutFunc(shared_from_this()) : false;
        }

        void MyChannel::setOnTimeoutFunc(MF::Server::MyChannel::OnTimeoutFunc &&func) {
            onTimeoutFunc = func;
        }

        EventLoop* MyChannel::getLoop() const {
            return loop;
        }

        void MyChannel::setLoop(EventLoop *loop) {
            MyChannel::loop = loop;
        }

        uint32_t MyChannel::getLastReceiveTime() const {
            return lastReceiveTime;
        }

        MyTcpChannel::MyTcpChannel(Socket::MySocket *socket) : MyChannel(socket) {}

        int32_t MyTcpChannel::onRead() {
            int32_t rv = 0;
            uint32_t len = 1024;
            while (true) {
                char *buf = readBuf->writeable(len);
                auto read = socket->read(buf, len);
                if (read > 0) {
                    readBuf->moveWriteable(static_cast<uint32_t >(read));
                    rv += read;
                } else if (read == 0) {
                    rv = 0; //对端关闭了
                    break;
                } else {
                    rv = -1; //读取失败了
                    break;
                }
            }
            lastReceiveTime = MyTimeProvider::now(); //设置最后一次接收到消息的时间
            return rv;
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

        uint32_t MyTcpChannel::sendResponse(const char *buf, uint32_t length) {
            std::lock_guard<std::mutex> guard(writeBufMutex); //尝试加锁
            char* tmp = this->writeBuf->getWriteableAndMove(length);
            if (tmp != nullptr) {
                memcpy(tmp, buf, length); //拷贝数据

                //发送可写请求
                this->writeWatcher->signal();
            }
            return length;
        }

        void MyTcpChannel::close() {
            if (socket != nullptr) {
                delete(socket);
                socket = nullptr;
            }

            if (readWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(readWatcher);
                readWatcher = nullptr;
            }

            if (writeWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(writeWatcher);
                writeWatcher = nullptr;
            }

            if (timeoutWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(timeoutWatcher);
                timeoutWatcher = nullptr;
            }
        }

        MyUdpChannel::MyUdpChannel(MF::Socket::MySocket *socket, const std::string& ip, uint16_t port)
        : MyChannel(socket){
            this->ip = ip;
            this->port = port;

            //重新生成uid
            this->uid = createUid(ip, port);
        }

        int32_t MyUdpChannel::onRead() {
            uint32_t len = g_max_udp_packet_length;
            char* buf = this->readBuf->writeable(len);

            //读取数据
            sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            bzero(&addr, addrLen);
            int32_t readLen = this->socket->readFrom(reinterpret_cast<sockaddr*>(&addr), &addrLen, buf, len);
            if (readLen <= 0) {
                LOG(ERROR) << "read data fail, readLen: " << readLen << std::endl;
                return -1;
            }

            //移动指针
            readBuf->moveWriteable(static_cast<uint32_t >(static_cast<uint32_t >(readLen)));
            lastReceiveTime = MyTimeProvider::now(); //设置最后一次接收到消息的时间
            return readLen;
        }

        int32_t MyUdpChannel::onWrite() {
            std::lock_guard<std::mutex> guard(writeBufMutex); //尝试加锁
            int32_t rv = 0;
            do {
                //1. 获取可发送的长度
                uint32_t len = this->writeBuf->getReadableLength();
                uint32_t flagLen = sizeof(uint32_t);
                //数据包长度不够，不做发送
                if (len <= sizeof(uint32_t)) { //正常来说不会走进来
                    break;
                }

                //2. 获取即将发送的数据包长度
                char* buf = this->writeBuf->readable(&flagLen);
                uint32_t flag = 0;
                memcpy(&flag, buf, flagLen);
                if (flag > len - flagLen) { //编码出错了
                    rv = -1;
                    break;
                }
                this->writeBuf->moveReadable(flagLen); //移动指针

                //3. 发送数据
                uint32_t send = 0;
                buf = this->writeBuf->readable(&flag);
                while(send < flag) {
                    //需要确认多线程调用writeTo是否ok？
                    int32_t sent = socket->writeTo(
                            this->ip, this->port, static_cast<void*>(buf + send), flag - send);
                    if (sent <= 0) {
                        break;
                    }

                    send += sent; //保存已发送的数据长度
                }

                //2. 移动指针
                writeBuf->moveReadable(flag);
            } while(true);
            return rv; //返回发送结果
        }

        uint64_t MyUdpChannel::createUid(const std::string &ip, uint16_t port) {
            return static_cast<uint64_t > (std::hash<std::string>()(ip + ":" + MyCommon::tostr(port)));
        }

        uint32_t MyUdpChannel::sendResponse(const char *buf, uint32_t length) {
            std::lock_guard<std::mutex> guard(writeBufMutex); //尝试加锁
            //1. 开头的四个字节用于记录数据包长度
            uint32_t flagLen = sizeof(length);
            char* tmp = this->writeBuf->getWriteableAndMove(length + flagLen);
            if (tmp != nullptr) {
                memcpy(tmp, &length, flagLen);
                memcpy(tmp + flagLen, buf, length); //拷贝数据

                //发送可写请求
                this->writeWatcher->signal();
            }
            return length;
        }

        void MyUdpChannel::close() {
            if (writeWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(writeWatcher);
                writeWatcher = nullptr;
            }

            if (timeoutWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(timeoutWatcher);
                timeoutWatcher = nullptr;
            }
        }
    }
}
