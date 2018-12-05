//
// Created by mingweiliu on 2018/11/5.
//

#include "net/server/MyServant.h"
#include "net/client/MyCommunicator.h"
namespace MF {
    namespace Server{

        MyServant::MyServant(EventLoopManager *loopManager, MyDispatcher *dispatcher)
        : loopManager(loopManager), dispatcher( dispatcher) {
            loop = loopManager->getNextLoop();
        }

        int32_t MyServant::initialize(const MF::Server::ServantConfig &config) {
            this->config = config;

            //1. 初始化handler线程池
            this->handlerExecutor = new MyThreadExecutor<int32_t >(this->config.handlerThreadCount);
            return 0;
        }

        void MyServant::stopServant() {
            //关闭socket
            if (socket != nullptr) {
                socket->close(); //关闭socket
            }

            //关闭ioWatcher监听
            if (readWatcher != nullptr) {
                loop->remove(readWatcher);
            }
        }

        void MyServant::onRead(EV::MyWatcher* watcher) {
            LOG(INFO) << "MyServant::onRead" << std::endl;
            //读取数据并处理
            auto channel = doRead(watcher);
            if (channel != nullptr) {
                handlePackets(channel);
            }
        }

        void MyServant::onWrite(MF::EV::MyWatcher *watcher) {
            LOG(INFO) << "MyServant::onWrite" << std::endl;
            //1. 获取uid
            uint64_t uid = dynamic_cast<EV::MyAsyncWatcher*>(watcher)->getUid();

            //2. 检查channel是否有效
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr) {
                LOG(ERROR) << "channel is null, uid: " << uid << std::endl;
                return;
            }

            //3. 发送数据
            if(channel->onWrite() != 0) {
                LOG(ERROR) << "send data fail, close socket, uid: " << channel->getUid() << std::endl;
                loopManager->removeChannel(channel);
                return ;
            }
        }

        //链接超时了，需要断开
        void MyServant::onTimeout(MF::EV::MyWatcher *watcher) {
            LOG(INFO) << "MyServant::onTimeout"<<std::endl;

            //1. 获取uid
            uint64_t uid = dynamic_cast<EV::MyTimerWatcher*>(watcher)->getUid();

            //2. 检查channel是否有效
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr) {
                LOG(ERROR) << "channel is null, uid: " << uid << std::endl;
                return;
            }

            //3. 检查是否超时
//            if (MyTimeProvider::now() - channel->getLastReceiveTime() <= config.timeout) {
//                return;
//            }

            //3. 通知上层
            auto context = std::make_shared<MyContext>(std::weak_ptr<MyChannel>(channel));
            auto future = handlerExecutor->exec([this, context] () -> int32_t {
                //上层处理
                this->dispatcher->handleTimeout(context);
                return 0;
            });

            //4. 等待上层处理完毕
            auto rv = future.get();
            if (rv == 0) {
                //需要关闭socket
                LOG(INFO) << "close connection, uid: " << channel->getUid() << std::endl;
                loopManager->removeChannel(channel);
            }
        }

        void MyServant::handlePackets(shared_ptr<MF::Server::MyChannel> channel) {
            int32_t status = kPacketStatusIncomplete;
            do {
                //获取可读数据的长度
                uint32_t len = channel->getReadableLength();
                if (len <= 0) {
                    break; //所有数据都读完了
                }

                //获取数据
                char* buf = channel->getReadableBuffer(&len);
                if (buf == nullptr || len == 0) {
                    break; //所有数据都读完了
                }

                status = dispatcher->isPacketComplete(buf, len);
                if( status == kPacketStatusIncomplete) {
                    //数据包不完整，那么就等待下一次数据
                    LOG(INFO) << "packet incomplete, uid: " << channel->getUid() << ", length: " << len << std::endl;
                } else if (status == kPacketStatusError) {
                    //数据包出错了, 需std::move(要断开连接)
                    LOG(ERROR) << "packet error, uid: " << channel->getUid() << ", length: " << len << std::endl;
                    onReadError(channel);
                } else if (status == kPacketStatusComplete) {
                    onReadComplete(channel, buf, len);
                }
            } while(status == kPacketStatusComplete);
        }

        void MyServant::onReadComplete(std::shared_ptr<MyChannel> channel, const char* buf, uint32_t len) {
            //数据包完整了，那么需要调用dispatcher来处理
            //获取完成数据包长度
            uint32_t packetLen = dispatcher->getPacketLength(buf, len);
            if (packetLen <= 0) {
                LOG(ERROR) << "packet length is invalid, uid: " << channel->getUid() << std::endl;
                //关闭channel
                loopManager->removeChannel(channel);
                return ;
            }
            //获取完整的数据包
            std::unique_ptr<Buffer::MyIOBuf> packet = channel->fetchPacket(packetLen);
            if (packet == nullptr) {
                LOG(ERROR) << "fetch packet fail, uid: " << channel->getUid() << ", length: " << packetLen << std::endl;
                return;
            }
            //获取context
            auto context = std::make_shared<MyContext>(std::weak_ptr<MyChannel>(channel));
            Buffer::MyIOBuf* iobuf = packet.release();

            //处理数据包
            this->handlerExecutor->exec([this, iobuf, context]() -> int32_t {
                //重新构造unique_ptr
                std::unique_ptr<Buffer::MyIOBuf> req(iobuf);
                return this->dispatcher->handlePacket(req, context);
            });
        }

        void MyServant::onReadError(shared_ptr<MF::Server::MyChannel> channel) {
            //1. 回掉到上层
            auto context = std::make_shared<MyContext>(std::weak_ptr<MyChannel>(channel));
            auto future = handlerExecutor->exec([this, context] () -> int32_t {
                return this->dispatcher->handleClose(context);
            });

            future.get(); //等待处理结束
            loopManager->removeChannel(channel);
        }

        void MyServant::registerServant() {
            auto proxy = Client::MyCommunicator::GetInstance()
                    ->getServantProxy<Route::MyRouteProxy>(routeServantName);
            if (proxy == nullptr) {
                LOG(ERROR) << "cant find proxy, routeName: " << routeServantName << std::endl;
                return;
            }
            auto wp = std::weak_ptr<Route::MyRouteProxy>(proxy);
            proxy->runAfterConnected([wp, this] () -> void {
                LOG(INFO) << "start register servant" << std::endl;
                auto proxy = wp.lock();
                if (proxy != nullptr) {
                    auto req = std::unique_ptr<RegisterReq>(new RegisterReq());
                    req->set_version(this->config.version);
                    req->mutable_nodeinfo()->set_host(this->config.host);
                    req->mutable_nodeinfo()->set_port(this->config.port);
                    req->mutable_nodeinfo()->set_servername("");
                    req->mutable_nodeinfo()->set_servantname(this->config.name);
                    req->mutable_nodeinfo()->set_status(kNodeStatusOnline);
                    auto rsp = proxy->registerServant(std::move(req));
                    if (rsp == nullptr) {
                        LOG(ERROR) << "register servant fail, name: " << config.name
                                   << ", host: " << config.host
                                   << ", port: " << config.port << std::endl;
                        return;
                    }

                    //注册成功
                    LOG(INFO) << "register servant success, name: " << config.name
                              << ", host: " << config.host
                              << ", port: " << config.port
                              << ", code: " << rsp->code()
                              << ", nodeId: " << rsp->nodeid()
                              << std::endl;
                }
            });
        }

        int32_t MyTcpServant::startServant() {
            std::string host = config.host;
            uint16_t port = config.port;
            //1. 构造socket
            socket = new Socket::MySocket();
            if (socket->socket(PF_INET, SOCK_STREAM, 0) != 0) {
                LOG(ERROR) << "create socket fail, host: " << host << ", port: " << port
                << ", error: " << strerror(errno) << std::endl;
                return -1; //初始化失败
            }

            //设置属性, 异步socket
            socket->setNonBlock();
            socket->setReusePort(true); //重用端口

            //2. bind
            if (socket->bind(host, port) != 0) {
                LOG(ERROR) << "bind socket fail, host: " << host << ", port: " << port
                << ", error: " << strerror(errno) << std::endl;
                return -1; //初始化失败
            }

            //3. listen
            if (socket->listen(g_default_listen_backlog) != 0) {
                LOG(ERROR) << "listen socket fail, host: " << host << ", port: " << port
                        << ", error: " << strerror(errno) << std::endl;
                return -1; //初始化失败
            }

            //4. 构造watcher
            readWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyTcpServant::onAccept, this, std::placeholders::_1), socket->getfd(), EV_READ);
            loop->add(readWatcher);

            //5. 上报节点
            if (this->config.name != this->routeServantName) {
                registerServant();
            }
            return 0;
        }

        //on accept
        void MyTcpServant::onAccept(EV::MyWatcher* watcher) {
            auto accepted = new Socket::MySocket();
            if(socket->accept(accepted) != 0) {
                LOG(ERROR) << "accept client fail" << std::endl;
                return ;
            }

           createChannel(accepted);
        }

        MyTcpServant::MyTcpServant(EventLoopManager *loopManager, MyDispatcher *dispatcher)
        : MyServant(loopManager, dispatcher) {}

        void MyTcpServant::createChannel(Socket::MySocket *socket) {
            //构造Channel
            auto channel = std::make_shared<MyTcpChannel>(socket);

            //将新连接加入到evloop中
            auto ioLoop = loopManager->getByUid(channel->getUid()); //根据uid获取evloop
            if (ioLoop == nullptr) {
                LOG(ERROR) << "get next loop fail" << std::endl;
                return ;
            }

            //构造watcher
            EV::MyIOWatcher* ioWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyTcpServant::onRead, this, std::placeholders::_1), socket->getfd(), EV_READ);
            EV::MyAsyncWatcher* writeWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyAsyncWatcher>(
                    std::bind(&MyTcpServant::onWrite, this, std::placeholders::_1));
            auto timeoutWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyTimerWatcher>(
                    std::bind(&MyTcpServant::onTimeout, this, std::placeholders::_1), config.timeout, 0);

            //设置ev data
            ioWatcher->setUid(channel->getUid());
            writeWatcher->setUid(channel->getUid());
            timeoutWatcher->setUid(channel->getUid());

            //开启事件监听
            ioLoop->add(ioWatcher);
            ioLoop->add(writeWatcher);
            ioLoop->add(timeoutWatcher);

            //保存watcher
            channel->setReadWatcher(ioWatcher);
            channel->setWriteWatcher(writeWatcher);
            channel->setTimeoutWatcher(timeoutWatcher);

            //保存iothread
            channel->setLoop(ioLoop);
            loopManager->addChannel(channel);

            LOG(INFO) << "client connected, ip: " << socket->getRemoteHost()
            << ", port: " << socket->getRemotePort()
            << ", uid: " << channel->getUid() << std::endl;
        }

        shared_ptr<MyChannel> MyTcpServant::doRead(EV::MyWatcher *watcher) {
            //1. 获取uid
            uint64_t uid = dynamic_cast<EV::MyIOWatcher*>(watcher)->getUid();

            //2. 获取channel
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr) {
                LOG(ERROR) << "find channel fail, uid: " << uid << std::endl;
                return nullptr;
            }

            //有数据到到达了，重新设置超时定时器
            channel->resetTimer(config.timeout);

            //4. 使用channel读取数据
            auto rv = channel->onRead();
            if (rv < 0) {
                if (errno != EAGAIN) { //读取失败，并且不是数据读完，那么就关闭socket
                    LOG(ERROR) << "read error, uid: " << uid << std::endl;
                    //读取失败，需要关闭channel
                    onReadError(channel);
                    return nullptr;
                }
            } else if (rv == 0) {
                LOG(INFO) << "connection close by remote, uid: " << uid << std::endl;
                onReadError(channel);
                return nullptr;
            }
            return channel;
        }


        MyUdpServant::MyUdpServant(
                MF::Server::EventLoopManager *loopManager
                , MF::Server::MyDispatcher *dispatcher)
                : MyServant(loopManager, dispatcher){

        }

        int32_t MyUdpServant::startServant() {
            std::string host = config.host;
            uint16_t port = config.port;
            //1. 构造socket
            socket = new Socket::MySocket();
            if (socket->socket(PF_INET, SOCK_DGRAM, 0) != 0) {
                LOG(ERROR) << "create socket fail, host: " << host << ", port: " << port
                           << ", error: " << strerror(errno) << std::endl;
                return -1; //初始化失败
            }

            //设置属性, 异步socket
            socket->setNonBlock();
            socket->setReusePort(true); //重用端口

            //2. bind
            if (socket->bind(host, port) != 0) {
                LOG(ERROR) << "bind socket fail, host: " << host << ", port: " << port
                           << ", error: " << strerror(errno) << std::endl;
                return -1; //初始化失败
            }

            //3. 构造watcher
            readWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyIOWatcher>(
                    std::bind(&MyUdpServant::onRead, this, std::placeholders::_1), socket->getfd(), EV_READ);
            loop->add(readWatcher);

            //4. 注册节点


            return 0;
        }

        std::shared_ptr<MyUdpChannel> MyUdpServant::createChannel(
                Socket::MySocket *socket, const std::string& ip, uint16_t port) {
            //构造Channel
            auto channel = std::make_shared<MyUdpChannel>(socket, ip, port);

            //将新连接加入到evloop中
            auto ioLoop = loopManager->getByUid(channel->getUid()); //根据uid获取evloop
            if (ioLoop == nullptr) {
                LOG(ERROR) << "get next loop fail" << std::endl;
                return nullptr;
            }

            //构造watcher
            EV::MyAsyncWatcher* writeWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyAsyncWatcher>(
                    std::bind(&MyUdpServant::onWrite, this, std::placeholders::_1));
            auto timeoutWatcher = EV::MyWatcherManager::GetInstance()->create<EV::MyTimerWatcher>(
                    std::bind(&MyUdpServant::onTimeout, this, std::placeholders::_1), config.timeout, 0);

            //设置ev data
            writeWatcher->setUid(channel->getUid());
            timeoutWatcher->setUid(channel->getUid());

            //开启事件监听
            ioLoop->add(writeWatcher);
            ioLoop->add(timeoutWatcher);

            //保存watcher
            channel->setWriteWatcher(writeWatcher);
            channel->setTimeoutWatcher(timeoutWatcher);

            //保存iothread
            channel->setLoop(ioLoop);
            loopManager->addChannel(channel);

            LOG(INFO) << "client connected, ip: " << ip << ", port: " << port
            << ", uid: " << channel->getUid() << std::endl;

            return channel;
        }

        shared_ptr<MyChannel> MyUdpServant::doRead(EV::MyWatcher *watcher) {
            //1. 检查消息来源
            sockaddr_in addr = {0};
            uint32_t addrLen = sizeof(addr);

            if(socket->peekFrom(reinterpret_cast<sockaddr*>(&addr), &addrLen) <= 0) {
                if (errno != EAGAIN) {
                    LOG(ERROR) << "read error, fd: " << socket->getfd() << std::endl;
                }

                return nullptr;
            }

            //2. 生成ui
            std::string ip = inet_ntoa(addr.sin_addr);
            uint16_t port = ntohs(addr.sin_port);
            auto uid = MyUdpChannel::createUid(ip, port);

            //3. 根据uid查找对应的channel
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr
                && (channel = createChannel(this->socket, ip, port)) == nullptr) {
                LOG(ERROR) << "create udp channel fail, uid: " << uid
                           << ", ip: " << ip << ", port: " << port << std::endl;
                return nullptr;
            }

            //有数据到到达了，重新设置超时定时器
            channel->resetTimer(config.timeout);

            //4. 读取数据
            int32_t readLen = channel->onRead();
            if (readLen <= 0) {
                if (errno != EAGAIN) {
                    LOG(ERROR) << "read error, uid: " << channel->getUid() << std::endl;
                    onReadError(channel);
                    return nullptr;
                }
            }

            return channel;
        }
    }
}

