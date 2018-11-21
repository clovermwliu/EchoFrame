//
// Created by mingweiliu on 2018/11/14.
//

#include "net/client/MyProxy.h"
#include "net/client/ClientLoop.h"

namespace MF {
    namespace Client {

        MyProxy::MyProxy(ClientLoopManager* loops) : loops(loops) {
        }

        MyProxy::~MyProxy() {
        }

        void MyProxy::update(const MF::Client::ProxyConfig &config) {
            this->config = config;
        }

        int32_t MyProxy::addTcpClient(const MF::Client::ClientConfig &config) {
            //1. 构造client
            auto client = std::make_shared<MyTcpClient>(hash(getServantName()) % 0xFFF);

            //2. 连接
            auto loop = loops->getByServantId(client->getServantId());
            auto future = client->asyncConnect(config, loop).share();
            auto self = shared_from_this();
            loop->RunInThreadAfterDelay([self, client, future, config] () -> void {
                if(future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    LOG(ERROR) << "connect timeout, host: " << config.host << ", port:" << config.port << std::endl;
                    return;
                }
                //promise返回了
                auto rv = future.get();
                if (rv != 0) { //链接失败
                    LOG(INFO) << "tcp client connect success, host: "
                              << config.host << ", port: " << config.port
                              << " ,error: " << strerror(rv) << std::endl;
                    return;
                }

                //connect 成功
                LOG(INFO) << "tcp client connect success, host: "
                          << config.host << ", port: " << config.port << std::endl;

                //构造read watcher
                auto readWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                        std::bind(&MyProxy::onRead, self.get(), std::placeholders::_1), client->getUid(), EV_READ);

                //watcher保存到事件循环
                client->getLoop()->add(readWatcher);
                client->setReadWatcher(readWatcher); //保存readwatcher

                //保存client
                self->loops->addClient(client);
                self->clients.pushBack(std::weak_ptr<MyClient>(client));
            }, config.timeout);

            return 0;
        }

        int32_t MyProxy::addUdpClient(const MF::Client::ClientConfig &config) {
            return -1;
        }

        std::shared_ptr<MyClient> MyProxy::getClient() {
            //使用获取第一个client，并且在获取到之后将client保存到最后
            std::shared_ptr<MyClient> client = nullptr;
            while(clients.empty()) {
                std::weak_ptr<MyClient> c;
                if (!clients.popFront(c)) {
                    continue;
                }
                client = c.lock();
                if (client != nullptr) { //找到了可用的连接
                    clients.pushBack(c);
                    break;
                }
            }

            return client;
        }

        void MyProxy::onRead(MF::EV::MyWatcher *watcher) {
            //1. 获取uid
            uint32_t uid = dynamic_cast<EV::MyIOWatcher*>(watcher)->getUid();

            //2. 获取client
            auto client = loops->findClient(uid);
            if (client == nullptr) {
                LOG(ERROR) << "find client fail, uid: " << uid << std::endl;
                return;
            }

            char* buf = nullptr;
            uint32_t len = 0;
            int32_t rv = client->onRead(&buf, &len);
            if (rv < 0) {
                LOG(ERROR) << "read error, uid:" << uid << std::endl;
                loops->removeClient(client);
                return;
            } else if (rv == 0) {
                LOG(ERROR) << "server disconnected, uid: " << uid << std::endl;
                loops->removeClient(client);
                return;
            }

            //4. 处理读到的数据
            auto status = isPacketComplete(buf, len);
            if (status == kPacketStatusIncomplete) {
                //数据包不完整直接退出
            } else if(status == kPacketStatusError) {
                //数据包出错
                LOG(ERROR) << "receive error packet, uid: " << uid <<", close connection" << std::endl;
                loops->removeClient(client);
            } else {
                //数据包完整了，获取数据包长度
                uint32_t packetLength = getPacketLength(buf, len);
                if (packetLength <= 0) {
                    LOG(ERROR) << "packet length is invalid, uid: " << uid << ", close connection" << std::endl;
                    return;
                }

                //获取完整数据包
                std::unique_ptr<Buffer::MyIOBuf> iobuf = std::move(client->fetchPayload(packetLength));
                if (iobuf == nullptr) {
                    LOG(ERROR) << "fetch packet fail, uid: " << uid << ", close connection" << std::endl;
                    return;
                }

                //处理数据
                auto obuf = iobuf.release();
                handlePacket(std::unique_ptr<Buffer::MyIOBuf>(obuf));
            }
        }

        ServantProxy::ServantProxy(ClientLoopManager* loops)
        : MyProxy(loops) {
        }

        int32_t ServantProxy::initialize(const std::vector<ClientConfig>& config) {
            auto servantId = static_cast<uint32_t >(hash(getServantName()));
            //1. 获取loop
            auto loop = loops->getByServantId(servantId);
            if (loop == nullptr) {
                return -1;
            }

            auto self = dynamic_pointer_cast<ServantProxy>(shared_from_this());
            loop->RunInThread([self, loop]() -> void {
                //1. 检查删除的配置
                std::vector<std::shared_ptr<MyClient>> tmp;
                auto oldClients = loop->getClientMap();
                for (auto it = oldClients.begin(); it != oldClients.end(); ++it) {
                    bool rv = false;
                    for (auto k = self->config.clients.begin(); k != self->config.clients.end(); ++k) {
                        if (k->host == it->second->getConfig().host
                            && k->port == it->second->getConfig().port) {
                            rv = true;
                        }
                    }

                    if (!rv) { //该链接需要删掉
                        tmp.push_back(it->second);
                    }
                }
                for (auto it = tmp.begin(); it != tmp.end(); ++it) {
                    loop->removeClient(*it);
                }

                //2. 初始化所有的配置
                for (auto it = self->config.clients.begin(); it != self->config.clients.end(); ++it) {
                    //2. 检查client是否存在
                    auto client = loop->findClient(it->host, it->port);
                    if (client != nullptr) { //client已存在，那么就跳过
                        continue;
                    }
                    if (it->clientType == kClientTypeTcp) {
                        if (self->addTcpClient(*it) != 0) {
                            LOG(ERROR) << "add tcp client fail" << std::endl;
                        }
                    } else if(it->clientType == kClientTypeUdp) {
                        if (self->addUdpClient(*it) != 0) {
                            LOG(ERROR) << "add udp client fail" << std::endl;
                        }
                    }
                }
            });

            return 0;
        }

        void ServantProxy::handlePacket(const std::unique_ptr<Buffer::MyIOBuf> &iobuf) {
           //1. 解析数据包
           auto magicMsg = std::unique_ptr<Protocol::MyMagicMessage>(new Protocol::MyMagicMessage());
           magicMsg->decode(iobuf);

           //2. 查找Client
           auto clientId = uint32_t(magicMsg->getRequestId() >> 32);
           auto client = loops->findClient(clientId);
           if (client == nullptr) {
               LOG(ERROR) << "find client fail, requestId: " << magicMsg->getRequestId() << std::endl;
               return;
           }

           //3. 查找对应的Request
           auto request = client->findRequest(magicMsg->getRequestId());
           if (request == nullptr) {
               LOG(ERROR) << "find request fail, requestId: " << magicMsg->getRequestId() << std::endl;
               return ;
           }
           auto m = magicMsg.release();
           auto self = dynamic_pointer_cast<ServantProxy>(shared_from_this());
           this->handlerExecutor->exec([self, m, request] () -> int32_t{
              auto mm = std::unique_ptr<Protocol::MyMagicMessage>(m);
               //4. 解码消息内容
               auto payload = self->decode(mm->getPayload());
               if (payload == nullptr) {
                   LOG(ERROR) << "decode message fail, requestId: " << mm->getRequestId() << std::endl;
                   return kClientResultFail;
               }

               //5. 处理响应
               return request->doSuccessAction(std::move(payload));
           });
        }

        void ServantProxy::update(const ProxyConfig &config) {
            MyProxy::update(config);

            this->initialize(this->config.clients);
        }
    }
}
