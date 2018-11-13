//
// Created by mingweiliu on 2018/11/5.
//

#ifndef MYFRAMEWORK2_MYSERVANT_H
#define MYFRAMEWORK2_MYSERVANT_H

#include "net/MyGlobal.h"
#include "net/server/EventLoop.h"
#include "net/socket/MySocket.h"
#include "net/server/MyDispatcher.h"
#include "net/buffer/MySKBuffer.h"
#include "net/server/MyChannel.h"
#include "util/MyThreadPool.h"

namespace MF {
    namespace Server{

        struct ServantConfig{
            std::string name; //servant 名称
            std::string host; //绑定的host
            uint16_t port; //绑定的端口
            uint64_t timeout; //client 超时断连时间(s)
            uint32_t handlerThreadCount; //handler线程数
        };

        /**
         * servant 类，用于适配loop、socket、dispatcher
         * servant 用于接受新连接、维护心跳已经检测断开链接等功能
         */
        class MyServant {
        public:
            /**
             * 构造函数
             * @param loop  loop
             * @param socket socket
             * @param dispatcher
             */
            MyServant(EventLoopManager *loopManager, MyDispatcher *dispatcher);

            /**
             * 析构函数
             */
            virtual ~MyServant();

            /**
             * 初始化Servant
             * @param config 配置
             * @return
             */
            virtual int32_t initialize(const ServantConfig& config);

            /**
             * 启动servant
             * @return 0 成功 其他失败
             */
            virtual int32_t startServant() = 0;

            /**
             * 停止
             */
            void stopServant();

        protected:
            /**
             * 构造一个Channel
             * @return channel
             */
            virtual void createChannel(Socket::MySocket* socket) = 0;
        protected:

            ServantConfig config; //servant 配置

            EventLoopManager* loopManager{nullptr}; //事件循环
            EventLoop* loop{nullptr}; //servant 所在事件循环
            MyDispatcher* dispatcher {nullptr}; //消息分发器

            //socket相关
            Socket::MySocket* socket {nullptr}; //socket
            EV::MyIOWatcher* readWatcher {nullptr}; //ioWatcher

            MyThreadExecutor<int32_t >* handlerExecutor; //handler的执行线程池
        };

        /**
         * tcp servant
         */
        class MyTcpServant : public MyServant {
        public:
            /**
             * 构造函数
             * @param loopManager loop manager
             * @param dispatcher dispatcher
             */
            MyTcpServant(EventLoopManager *loopManager, MyDispatcher *dispatcher);

/**
             * 开启servant
             * @param host host
             * @param port port
             * @return 结果
             */
            int32_t startServant() override;

        protected:
            /**
             * 接收新的链接
             * @return
             */
            void onAccept(EV::MyWatcher* watcher);

            /**
             * 有数据可以读取
             */
            void onRead(EV::MyWatcher* watcher);

            /**
             * 需要发送数据
             * @param watcher watcher
             */
            void onWrite(EV::MyWatcher* watcher);

            /**
             * 连接超时
             * @param watcher watcher
             */
            void onTimeout(EV::MyWatcher* watcher);

            void createChannel(Socket::MySocket *socket) override;

            /**
             * 数据包完整
             */
            void onReadComplete(std::shared_ptr<MyChannel> channel, const char* buf, uint32_t len) ;

            /**
             * 数据包读取出错
             * @param channel
             */
            void onReadError(std::shared_ptr<MyChannel> channel);

        protected:
        };
    }
}


#endif //MYFRAMEWORK2_MYSERVANT_H
