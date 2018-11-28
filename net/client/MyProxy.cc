//
// Created by mingweiliu on 2018/11/14.
//

#include "net/client/MyProxy.h"
#include "net/client/ClientLoop.h"
#include "util/MyTimeProvider.h"

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
            //保存client
            loops->addClient(client);

            //2. 连接
            auto loop = loops->getByServantId(client->getServantId());
            auto self = shared_from_this();
            auto future = client->asyncConnect(config, loop, [self] (std::shared_ptr<MyClient> client) {
                if (client->connected()) {
                    //构造read watcher
                    auto readWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                            std::bind(&MyProxy::onRead, self.get(), std::placeholders::_1), client->getFd(), EV_READ);
                    //设置uid
                    readWatcher->setUid(client->getUid());

                    //watcher保存到事件循环
                    client->getLoop()->add(readWatcher);
                    client->setReadWatcher(readWatcher); //保存readwatcher
                    self->clients.pushBack(std::weak_ptr<MyClient>(client));
                } else {
                    //连接失败，那么就10秒后再试一次
                    if (client->getConfig().autoReconnect) {
                        client->getLoop()->RunInThreadAfterDelay([client] (EV::MyTimerWatcher*) -> void {
                            client->reconnect();
                        }, self->config.reconnectInterval);
                    }
                }
            });

            return future.wait_for(std::chrono::seconds(0))
                    == std::future_status::ready ? future.get() : 0;
        }

        int32_t MyProxy::addUdpClient(const MF::Client::ClientConfig &config) {
            //1. 构造client
            auto client = std::make_shared<MyUdpClient>(hash(getServantName()) % 0xFFF);
            //保存client
            loops->addClient(client);

            //2. 连接
            auto loop = loops->getByServantId(client->getServantId());
            auto self = shared_from_this();
            if(client->connect(config, loop) != 0) {
                LOG(ERROR) << "connect fail, host: " << config.host << ", port: " << config.port << std::endl;
                return -1;
            }

            //构造read watcher
            auto readWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyProxy::onRead, self.get(), std::placeholders::_1), client->getFd(), EV_READ);
            //设置uid
            readWatcher->setUid(client->getUid());

            //watcher保存到事件循环
            client->getLoop()->add(readWatcher);
            client->setReadWatcher(readWatcher); //保存readwatcher
            self->clients.pushBack(std::weak_ptr<MyClient>(client));
        }

        std::shared_ptr<MyClient> MyProxy::getClient() {
            //使用获取第一个client，并且在获取到之后将client保存到最后
            std::shared_ptr<MyClient> client = nullptr;
            while(!clients.empty()) {
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
            uint64_t uid = dynamic_cast<EV::MyIOWatcher*>(watcher)->getUid();

            //2. 获取client
            auto client = loops->findClient(uid);
            if (client == nullptr) {
                LOG(ERROR) << "find client fail, uid: " << uid << std::endl;
                return;
            }

            int32_t rv = client->onRead();
            if (rv < 0) {
                if (errno != EAGAIN) {
                    LOG(ERROR) << "read error, uid:" << uid << std::endl;
                    onDisconnect(client);
                    return;
                }
            } else if (rv == 0) {
                LOG(INFO) << "connection closed by remote, uid: " << uid << std::endl;
                onDisconnect(client);
                return;
            }

            //3. 循环获取所有的数据
            int32_t status = kPacketStatusIncomplete;
            do {
                //获取可读数据的长度
                uint32_t len = client->getReadableLength();
                if (len <= 0) {
                    break; //所有数据都读完了
                }

                //获取数据
                char* buf = client->getReadableBuffer(&len);
                if (buf == nullptr || len == 0) {
                    break; //所有数据都读完了
                }

                //4. 处理读到的数据
                status = isPacketComplete(buf, len);
                if (status == kPacketStatusIncomplete) {
                    //数据包不完整直接退出
                    LOG(INFO) << "receive packet incomplete, uid: " << client->getUid() << ", length: " << len << std::endl;
                } else if(status == kPacketStatusError) {
                    //数据包出错
                    LOG(ERROR) << "receive error packet, uid: " << client->getUid() <<", close connection" << std::endl;
                    onDisconnect(client);
                } else {
                    //数据包完整了，获取数据包长度
                    uint32_t packetLength = getPacketLength(buf, len);
                    if (packetLength <= 0) {
                        LOG(ERROR) << "packet length is invalid, uid: " << client->getUid() << ", close connection" << std::endl;
                        break;
                    }

                    //获取完整数据包
                    std::unique_ptr<Buffer::MyIOBuf> iobuf = std::move(client->fetchPayload(packetLength));
                    if (iobuf == nullptr) {
                        LOG(ERROR) << "fetch packet fail, uid: " << client->getUid() << ", close connection" << std::endl;
                        break;
                    }

                    //处理数据
                    auto obuf = iobuf.release();
                    handlePacket(std::unique_ptr<Buffer::MyIOBuf>(obuf));
                }
            }while (status == kPacketStatusComplete);

        }

        void MyProxy::onDisconnect(const std::shared_ptr<MyClient>& client) {

            //不需要自动重连
            if (!client->getConfig().autoReconnect) {
                loops->removeClient(client);
                return;
            }

            //x秒之后重连
            auto self = shared_from_this();
            client->disconnect(); //先断开链接
            client->getLoop()->RunInThreadAfterDelay([client] (EV::MyTimerWatcher*) -> void {
                client->reconnect();
            }, self->config.reconnectInterval);

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
           //将request从client 中删除
           client->removeRequest(magicMsg->getRequestId());

           //处理消息
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

        void ServantProxy::handleMessage(const char* buf, uint32_t len, std::shared_ptr<MyClient> client) {

        }
    }
}
