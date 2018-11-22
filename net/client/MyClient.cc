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

        MyTcpClient::MyTcpClient(uint16_t servantId) : MyClient(servantId){
            socket->socket(PF_INET, SOCK_STREAM, 0);
        }

        int32_t MyTcpClient::connect(const ClientConfig &config, ClientLoop* loop) {
            
            //调用异步接口
            auto future = asyncConnect(config, loop);
            
            //等待任务完成
            if (future.wait_for(std::chrono::seconds(config.timeout)) != std::future_status::ready) {
                LOG(ERROR) << "connect timeout, host: " << config.host << ", port: " << config.port << std::endl;
                return -1;
            }

            //返回结果
            return future.get();
        }

        std::future<int32_t> MyTcpClient::asyncConnect(const ClientConfig &config, ClientLoop* loop) {
            this->config = config; //保存配置
            this->loop = loop; //保存loop

            //1. 连接
            socket->setNonBlock(); //设置异步
            if (socket->connect(config.host, config.port) != 0) {
                LOG(ERROR) << "connect fail, host: " << config.host << ", port: " << config.port << std::endl;
                connectPromise.set_value(-1);
                return connectPromise.get_future();
            }

            //2. 构造read watcher
            connectWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyTcpClient::onConnect, this, std::placeholders::_1), socket->getfd(), EV_WRITE);

            this->loop->add(connectWatcher);
            this->connectTime = MyTimeProvider::now();
            return connectPromise.get_future();
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
                socket->setBlock();
            } else {
                LOG(ERROR) << "connect fail, host: " << config.host << ", port: " << config.port << std::endl;
            }
            connectPromise.set_value(err); //future返回结果

            //关闭watcher
            EV::MyWatcherManager::GetInstance()->destroy(connectWatcher);
            connectWatcher = nullptr;
            socket->setConnected(); //设置socket已连接
        }

        int32_t MyTcpClient::onRead(char** buf, uint32_t* len) {
            if (!socket->connected()) {
                return 0;
            }

            *len = 1024;
            *buf = readBuffer->writeable(*len);
            auto rv = socket->read(*buf, *len);
            if (rv < 0 && errno == EAGAIN) {
                rv = 0;
            }
            return rv;
        }
    }
}
