//
// Created by mingweiliu on 2018/11/14.
//

#include "net/client/MyClient.h"
#include "net/client/ClientLoop.h"
#include "net/client/MyRequest.h"

namespace MF {
    namespace Client{

        /**
         * 增加request
         * @param request request
         */
        void MyClient::addRequest(const std::shared_ptr<MyBaseRequest>& request ) {
            requests[request->getRequestId()] = request;
        }

        /**
         * 删除一个request
         * @param request request
         */
        void MyClient::removeRequest(uint64_t requestId) {
            requests.erase(requestId);
        }

        /**
         * 查询request
         * @param requestId requestId
         * @return request
         */
        std::shared_ptr<MyBaseRequest> MyClient::findRequest(uint64_t requestId) {
            return requests.find(requestId) != requests.end() ? requests[requestId] : nullptr;
        }

        uint32_t MyClient::getConnectTime() const {
            return connectTime;
        }

        void MyClient::whenRequestTimeout(std::function<void()> &&pred, uint64_t requestId) {
            auto self = std::weak_ptr<MyClient>(shared_from_this());
            auto func = [pred, requestId, self] (EV::MyTimerWatcher*) {
                pred(); //执行代码
                //删除request
                auto s = self.lock();
                if (s != nullptr) {
                    s->removeRequest(requestId);
                }
            };

            loop->RunInThreadAfterDelay(func, config.timeout);
        }

        MyTcpClient::MyTcpClient(uint16_t servantId) : MyClient(servantId){
            socket->socket(PF_INET, SOCK_STREAM, 0);
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

        std::future<int32_t > MyTcpClient::reconnect() {
            //1. 断开链接
            disconnect();

            //2. 重新链接
            socket = new Socket::MySocket();
            if (socket->socket(AF_INET, SOCK_STREAM, 0) != 0) {
                LOG(ERROR) << "socket fail" << std::endl;
                connectPromise->set_value(-1); //连接失败
                return connectPromise->get_future();
            }

            //3. 连接
            return asyncConnect(this->config, this->loop, std::move(this->onConnectFunc));
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

        std::unique_ptr<Buffer::MyIOBuf> MyTcpClient::fetchPayload(uint32_t length) {
            char* buf = readBuffer->getReadableAndMove(&length);
            auto iobuf = Buffer::MyIOBuf::create(length);
            iobuf->write<char*>(buf, length);
            return iobuf;
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
    }
}
