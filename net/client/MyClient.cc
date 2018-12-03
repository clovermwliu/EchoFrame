//
// Created by mingweiliu on 2018/11/14.
//

#include "net/client/MyClient.h"
#include "net/client/ClientLoop.h"
#include "net/client/MySession.h"

namespace MF {
    namespace Client{

        /**
         * 增加request
         * @param request request
         */
        void MyClient::addSession(const std::shared_ptr<MyBaseSession> &request) {
            requests[request->getRequestId()] = request;
        }

        /**
         * 删除一个request
         * @param request request
         */
        void MyClient::removeSession(uint64_t requestId) {
            requests.erase(requestId);
        }

        /**
         * 查询request
         * @param requestId requestId
         * @return request
         */
        std::shared_ptr<MyBaseSession> MyClient::findSession(uint64_t requestId) {
            return requests.find(requestId) != requests.end() ? requests[requestId] : nullptr;
        }

        std::unique_ptr<Buffer::MyIOBuf> MyClient::fetchPayload(uint32_t length) {
            char* buf = readBuffer->getReadableAndMove(&length);
            auto iobuf = Buffer::MyIOBuf::create(length);
            iobuf->write<char*>(buf, length);
            return iobuf;
        }

        void MyClient::whenSessionTimeout(std::function<void()> &&pred, uint64_t requestId) {
            auto self = std::weak_ptr<MyClient>(shared_from_this());
            auto func = [pred, requestId, self] (EV::MyTimerWatcher*) {
                pred(); //执行代码
                //删除request
                auto s = self.lock();
                if (s != nullptr) {
                    s->removeSession(requestId);
                }
            };

            loop->RunInThreadAfterDelay(func, config.timeout);
        }

        MyTcpClient::MyTcpClient(uint16_t servantId) : MyClient(servantId){
            socket->socket(AF_INET, SOCK_STREAM, 0);
            uid = static_cast<uint32_t >(socket->getfd() << 16 | servantId);
        }

        MyTcpClient::~MyTcpClient() {
            disconnect();
        }

        int32_t MyTcpClient::connect(const ClientConfig &config, ClientLoop* loop) {
            
            //调用异步接口
            auto future = asyncConnect(config, loop, nullptr);
            
            //等待任务完成
            if (future.wait_for(std::chrono::seconds(config.timeout)) != std::future_status::ready) {
                LOG(ERROR) << "connect timeout, host: " << config.host << ", port: " << config.port << std::endl;
                return -1;
            }

            //返回结果
            return future.get();
        }

        void MyTcpClient::disconnect() {
            if (socket != nullptr) {
                delete(socket);
                socket = nullptr;
            }

            //停止readwatcher
            if (readWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(readWatcher);
                readWatcher = nullptr;
            }

            if (connectPromise != nullptr) {
                delete(connectPromise);
                connectPromise = nullptr;
            }

            if (readBuffer != nullptr) {
                readBuffer->reset();
            }
        }

        void MyTcpClient::reconnect() {
            //1. 断开链接
            disconnect();

            //2. 重新链接
            socket = new Socket::MySocket();
            if (socket->socket(AF_INET, SOCK_STREAM, 0) != 0) {
                LOG(ERROR) << "socket fail" << std::endl;
            }

            //3. 连接
            asyncConnect(this->config, this->loop, std::move(this->onConnectFunc));
        }

        std::future<int32_t> MyTcpClient::asyncConnect(
                const ClientConfig &config, ClientLoop* loop, OnConnectFunction && func) {
            this->config = config; //保存配置
            this->loop = loop; //保存loop
            this->onConnectFunc = func; //连接成功需要执行的回调
            this->connectPromise = new std::promise<int32_t >();

            //1. 连接
            socket->setNonBlock(); //设置异步
            if (socket->connect(config.host, config.port) != 0) {
                LOG(ERROR) << "connect fail, host: " << config.host << ", port: " << config.port << std::endl;
                connectPromise->set_value(-1);
                return connectPromise->get_future();
            }

            //2. 构造read watcher
            connectWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyTcpClient::onConnect, this, std::placeholders::_1), socket->getfd(), EV_WRITE);

            this->loop->add(connectWatcher);
            this->connectTime = MyTimeProvider::now();
            return connectPromise->get_future();
        }

        int32_t MyTcpClient::sendPayload(const char *buffer, uint32_t length) {
            auto self = shared_from_this();
            loop->RunInThread([self, buffer, length]() -> void {
                self->getSocket()->write((void*)buffer, length);
            });
            return 0;
        }

        void MyTcpClient::onConnect(EV::MyWatcher *watcher) {
            //1. 检查链接是否已经连上
            if (socket->connected()) {
                return;
            }
            //检查连接是否成功
            int32_t err = socket->getConnectResult();
            if (err == 0) {
                LOG(INFO) << "connect success, host: " << config.host << ", port: " << config.port << std::endl;
                socket->setConnected(); //设置已连接
            } else {
                LOG(ERROR) << "connect fail, host: " << config.host << ", port: " << config.port
                << ", error: " << strerror(err) << std::endl;
            }
            connectPromise->set_value(err); //future返回结果
            //关闭watcher
            EV::MyWatcherManager::GetInstance()->destroy(connectWatcher);
            connectWatcher = nullptr;
            //回调
            if (onConnectFunc) {
                onConnectFunc(shared_from_this());
            }
        }

        int32_t MyTcpClient::onRead() {
            if (!socket->connected()) {
                return -1;
            }
            uint32_t len = 1024;
            int32_t rv = 0;
            while (true) {
                char *buf = readBuffer->writeable(len);
                auto read = socket->read(buf, len);
                if (read > 0) {
                    readBuffer->moveWriteable(static_cast<uint32_t >(read));
                    rv += read;
                } else if (read == 0) {
                    rv = 0; //对端关闭了
                    break;
                } else {
                    rv = -1; //读取失败了
                    break;
                }
            }

            return rv;
        }

        int32_t MyTcpClient::sendPayload(std::unique_ptr<Buffer::MyIOBuf> iobuf) {
            auto self = shared_from_this();
            auto ptr = iobuf.release();
            loop->RunInThread([self, ptr]() -> void {
                std::unique_ptr<Buffer::MyIOBuf> tmpBuf(ptr);
                auto buffer = tmpBuf->readable();
                auto length = tmpBuf->getReadableLength();
                self->getSocket()->write(buffer, length);
            });

            return 0;
        }

        MyUdpClient::MyUdpClient(uint16_t servantId) : MyClient(servantId) {
            socket->socket(AF_INET, SOCK_DGRAM, 0);
            uid = static_cast<uint32_t >(socket->getfd() << 16 | servantId);
        }

        MyUdpClient::~MyUdpClient() {
            disconnect();
        }

        int32_t MyUdpClient::connect(const MF::Client::ClientConfig &config, MF::Client::ClientLoop *loop) {
            this->config = config; //保存配置
            this->loop = loop; //保存loop
            this->onConnectFunc = nullptr; //连接成功需要执行的回调
            socket->setNonBlock(); //设置异步
            this->connectTime = MyTimeProvider::now(); //设置连接时间
//            return socket->connect(config.host, config.port);
            return 0;
        }

        void MyUdpClient::disconnect() {
            if (socket != nullptr) {
                delete(socket);
                socket = nullptr;
            }

            if (readWatcher != nullptr) {
                EV::MyWatcherManager::GetInstance()->destroy(readWatcher);
                readWatcher = nullptr;
            }
        }

        void MyUdpClient::reconnect() {
            //断开链接
            disconnect();

            //重新链接
            connect(this->config, this->loop);
        }

        int32_t MyUdpClient::onRead() {
            int32_t rv = 0;
            uint32_t len = g_max_udp_packet_length;
            while(true) {
                char* buf = readBuffer->writeable(len);

                //读取数据
                sockaddr_in addr = {0};
                socklen_t addrLen = sizeof(addr);
                int32_t readLen = socket->readFrom(reinterpret_cast<sockaddr*>(&addr), &addrLen, buf, len);
                if (readLen <= 0) {
                    rv = -1;
                    break;
                }

                //移动指针
                readBuffer->moveWriteable(static_cast<uint32_t >(static_cast<uint32_t >(readLen)));

                //增加读取到的长度
                rv += readLen;
            }

            return rv;
        }

        int32_t MyUdpClient::sendPayload(const char *buffer, uint32_t length) {
            if (length > g_max_udp_packet_length) {
                LOG(ERROR) << "udp send buffer overflow, max: " << g_max_udp_packet_length << std::endl;
                return -1;
            }
            return socket->writeTo(config.host, config.port, const_cast<char*>(buffer), length);
        }

        int32_t MyUdpClient::sendPayload(std::unique_ptr<MF::Buffer::MyIOBuf> iobuf) {
            auto self = std::dynamic_pointer_cast<MyUdpClient>(shared_from_this());
            auto ptr = iobuf.release();
            loop->RunInThread([self, ptr]() -> void {
                std::unique_ptr<Buffer::MyIOBuf> tmpBuf(ptr);
                auto buffer = tmpBuf->readable();
                auto length = tmpBuf->getReadableLength();
                //打印发送失败
                if (length > g_max_udp_packet_length) {
                    LOG(ERROR) << "udp send buffer overflow, max: " << g_max_udp_packet_length << std::endl;
                    return;
                }
                if(self->socket->writeTo(self->config.host, self->config.port, buffer, length) <= 0) {
                    EAGAIN;
                    auto err = errno;
                    LOG(ERROR) << "send packet fail, host: " << self->config.host
                    << ",port: " << self->config.port << ", error: " << strerror(err) << ", err: " << err << std::endl;
                }
            });

            return 0;
        }
    }
}
