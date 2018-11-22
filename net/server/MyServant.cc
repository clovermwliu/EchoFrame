//
// Created by mingweiliu on 2018/11/5.
//

#include "net/server/MyServant.h"
namespace MF {
    namespace Server{

        MyServant::MyServant(EventLoopManager *loopManager, MyDispatcher *dispatcher)
        : loopManager(loopManager), dispatcher( dispatcher) {
            loop = loopManager->getNextLoop();
        }

        MyServant::~MyServant() {
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

        void MyTcpServant::onRead(EV::MyWatcher* watcher) {
            LOG(INFO) << "MyTcpServant::onConnect" << std::endl;
            //1. 获取uid
            uint32_t uid = dynamic_cast<EV::MyIOWatcher*>(watcher)->getUid();

            //2. 获取channel
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr) {
                LOG(ERROR) << "find channel fail, uid: " << uid << std::endl;
                return;
            }

            //4. 使用channel读取数据
            char* buf = nullptr;
            uint32_t len = 0;
            if (channel->onRead(&buf, &len) < 0) {
                LOG(ERROR) << "read error, uid: " << uid << std::endl;
                //读取失败，需要关闭channel
                onReadError(channel);
                return;
            }

            //5. 检查数据是否可用
            if (buf == nullptr || len == 0) {
                LOG(ERROR) << "read no data, uid: " << uid << std::endl;
                return; //没有数据直接返回
            }

            //6. 将数据交给dispatch
            int32_t rv = dispatcher->isPacketComplete(buf, len);
            if( rv == kPacketStatusIncomplete) {
                //数据包不完整，那么就等待下一次数据
                LOG(INFO) << "packet incomplete, uid: " << uid << ", length: " << len << std::endl;
            } else if (rv == kPacketStatusError) {
                //数据包出错了, 需std::move(要断开连接)
                LOG(ERROR) << "packet error, uid: " << uid << ", length: " << len << std::endl;
                onReadError(channel);
            } else if (rv == kPacketStatusComplete) {
                onReadComplete(channel, buf, len);
            }
        }

        void MyTcpServant::onWrite(MF::EV::MyWatcher *watcher) {
            LOG(INFO) << "MyTcpServant::onWrite" << std::endl;
            //1. 获取uid
            uint32_t uid = dynamic_cast<EV::MyAsyncWatcher*>(watcher)->getUid();

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
        void MyTcpServant::onTimeout(MF::EV::MyWatcher *watcher) {
            //1. 获取uid
            uint32_t uid = dynamic_cast<EV::MyTimerWatcher*>(watcher)->getUid();

            //2. 检查channel是否有效
            auto channel = loopManager->findChannel(uid);
            if (channel == nullptr) {
                LOG(ERROR) << "channel is null, uid: " << uid << std::endl;
                return;
            }

            //3. 检查是否超时
            if (MyTimeProvider::now() - channel->getLastReceiveTime() <= config.timeout) {
                return;
            }

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
                    std::bind(&MyTcpServant::onTimeout, this, std::placeholders::_1), config.timeout, config.timeout);

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

        void MyTcpServant::onReadComplete(std::shared_ptr<MyChannel> channel, const char* buf, uint32_t len) {
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

        void MyTcpServant::onReadError(shared_ptr<MF::Server::MyChannel> channel) {
            //1. 回掉到上层
            auto context = std::make_shared<MyContext>(std::weak_ptr<MyChannel>(channel));
            auto future = handlerExecutor->exec([this, context] () -> int32_t {
                return this->dispatcher->handleClose(context);
            });

            future.get(); //等待处理结束
            loopManager->removeChannel(channel);
        }
    }
}

